require "mkmf"

extension_name = 'yaz0/yaz0'
dir_config(extension_name)

libyaz0_src = Dir[File.join(__dir__, "../../libyaz0/src/libyaz0/**/*.c")].map{|x| File.expand_path(x)}

$srcs = libyaz0_src + ["ext_yaz0.c"]

$VPATH << File.expand_path(File.join(__dir__, "../../libyaz0/src/libyaz0"))
$INCFLAGS << " -I#{File.expand_path(File.join(__dir__, "../../libyaz0/include"))}"

create_makefile(extension_name)
