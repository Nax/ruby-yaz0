#include "ext_yaz0.h"
#include <stdio.h>
#include <stdint.h>

#define BUFSIZE 0x4000

static void ext_yaz0_stream_free(void*);

static VALUE class_yaz0_stream;
static VALUE class_yaz0_error;
static VALUE class_yaz0_error_bad_magic;
static VALUE class_yaz0_error_end_of_file;

static struct rb_data_type_struct type_yaz0_stream = {
    "yaz0_stream",
    { NULL, ext_yaz0_stream_free, NULL },
    NULL,
    NULL,
    RUBY_TYPED_FREE_IMMEDIATELY
};

static void
ext_yaz0_stream_free(void* s)
{
    yaz0Destroy((Yaz0Stream*)s);
}

static VALUE
ext_yaz0_stream_alloc(VALUE klass)
{
    Yaz0Stream* s;
    int ret;

    ret = yaz0Init(&s);
    if (ret == YAZ0_OUT_OF_MEMORY)
        rb_raise(rb_eNoMemError, "Out of memory");
    return TypedData_Wrap_Struct(klass, &type_yaz0_stream, s);
}

static void*
run_NoGVL(void* arg)
{
    return (void*)(intptr_t)yaz0Run((Yaz0Stream*)arg);
}

static VALUE
run(VALUE self, VALUE io_in, VALUE io_out, int compress, int size, int level)
{
    Yaz0Stream* s;
    VALUE buffer_in;
    VALUE buffer_out;
    VALUE tmp;
    int in_is_str;
    int ret;

    in_is_str = TYPE(io_in) == T_STRING;

    TypedData_Get_Struct(self, Yaz0Stream, &type_yaz0_stream, s);
    if (compress)
        yaz0ModeCompress(s, size, level);
    else
        yaz0ModeDecompress(s);

    /* Init the buffers */
    if (!in_is_str)
    {
        buffer_in = rb_str_new(NULL, 0);
        rb_str_resize(buffer_in, BUFSIZE);
        rb_gc_register_address(&buffer_in);
        rb_funcall(io_in, rb_intern("read"), 2, INT2FIX(BUFSIZE), buffer_in);
    }
    else
    {
        buffer_in = rb_obj_dup(io_in);
        rb_gc_register_address(&buffer_in);
    }
    yaz0Input(s, RSTRING_PTR(buffer_in), (uint32_t)RSTRING_LEN(buffer_in));

    buffer_out = rb_str_new(NULL, 0);
    rb_str_resize(buffer_out, BUFSIZE);
    rb_gc_register_address(&buffer_out);
    yaz0Output(s, RSTRING_PTR(buffer_out), BUFSIZE);

    for (;;)
    {
        ret = (int)(intptr_t)rb_thread_call_without_gvl(run_NoGVL, s, RUBY_UBF_IO, NULL);
        switch (ret)
        {
        case YAZ0_NEED_AVAIL_IN:
            /* Need more input */
            if (in_is_str)
            {
                rb_gc_unregister_address(&buffer_in);
                rb_gc_unregister_address(&buffer_out);
                rb_raise(class_yaz0_error_end_of_file, "Unexpected end of file");
            }

            tmp = rb_funcall(io_in, rb_intern("read"), 2, INT2FIX(BUFSIZE), buffer_in);
            if (tmp == Qnil)
            {
                rb_gc_unregister_address(&buffer_in);
                rb_gc_unregister_address(&buffer_out);
                rb_raise(class_yaz0_error_end_of_file, "Unexpected end of file");
            }
            yaz0Input(s, RSTRING_PTR(buffer_in), (uint32_t)RSTRING_LEN(buffer_in));
            break;
        case YAZ0_NEED_AVAIL_OUT:
            /* Need more output */
            rb_str_set_len(buffer_out, yaz0OutputChunkSize(s));
            rb_funcall(io_out, rb_intern("write"), 1, buffer_out);
            rb_str_set_len(buffer_out, BUFSIZE);
            yaz0Output(s, RSTRING_PTR(buffer_out), BUFSIZE);
            break;
        case YAZ0_BAD_MAGIC:
            rb_gc_unregister_address(&buffer_in);
            rb_gc_unregister_address(&buffer_out);
            rb_raise(class_yaz0_error_bad_magic, "Bad magic");
            break;
        case YAZ0_OK:
            goto end;
        }
    }

end:
    /* There might still be unflushed output */
    rb_str_set_len(buffer_out, yaz0OutputChunkSize(s));
    rb_funcall(io_out, rb_intern("write"), 1, buffer_out);

    rb_gc_unregister_address(&buffer_in);
    rb_gc_unregister_address(&buffer_out);

    return Qnil;
}

static VALUE
ext_yaz0_stream_raw_decompress(VALUE self, VALUE io_in, VALUE io_out)
{
    return run(self, io_in, io_out, 0, 0, 0);
}

static VALUE
ext_yaz0_stream_raw_compress(VALUE self, VALUE io_in, VALUE io_out, VALUE size, VALUE level)
{
    Check_Type(size, T_FIXNUM);
    Check_Type(level, T_FIXNUM);
    return run(self, io_in, io_out, 1, FIX2INT(size), FIX2INT(level));
}

void
Init_yaz0(void)
{
    VALUE mod;
    mod = rb_define_module("Yaz0");

    /* Error classes */
    class_yaz0_error = rb_define_class_under(mod, "Error", rb_eStandardError);
    class_yaz0_error_bad_magic = rb_define_class_under(mod, "BadMagicError", class_yaz0_error);
    class_yaz0_error_end_of_file = rb_define_class_under(mod, "EndOfFileError", class_yaz0_error);

    /* Stream */
    class_yaz0_stream = rb_define_class_under(mod, "Stream", rb_cObject);
    rb_define_alloc_func(class_yaz0_stream, ext_yaz0_stream_alloc);
    rb_define_method(class_yaz0_stream, "raw_decompress", ext_yaz0_stream_raw_decompress, 2);
    rb_define_method(class_yaz0_stream, "raw_compress", ext_yaz0_stream_raw_compress, 4);
}
