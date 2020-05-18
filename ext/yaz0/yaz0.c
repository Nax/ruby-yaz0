#include "yaz0.h"
#include <stdio.h>

static VALUE compress(VALUE self, VALUE str)
{
    Yaz0Buffer buffer;
    VALUE ret;

    if (TYPE(str) != T_STRING)
    {
        rb_raise(rb_eTypeError, "Expected a string");
        return Qnil;
    }

    yaz0BufferAlloc(&buffer, 16);
    yaz0Compress(&buffer, StringValuePtr(str), RSTRING_LEN(str));
    ret = rb_str_new(buffer.data, buffer.size);
    yaz0BufferFree(&buffer);

    return ret;
}

static VALUE decompress(VALUE self, VALUE str)
{
    Yaz0Buffer buffer;
    VALUE ret;

    if (TYPE(str) != T_STRING)
    {
        rb_raise(rb_eTypeError, "Expected a string");
        return Qnil;
    }

    yaz0BufferAlloc(&buffer, 16);
    yaz0Decompress(&buffer, StringValuePtr(str), RSTRING_LEN(str));
    ret = rb_str_new(buffer.data, buffer.size);
    yaz0BufferFree(&buffer);

    return ret;
}

void Init_yaz0(void)
{
    VALUE mod;
    mod = rb_define_module("Yaz0");
    rb_define_module_function(mod, "compress", &compress, 1);
    rb_define_module_function(mod, "decompress", &decompress, 1);
}
