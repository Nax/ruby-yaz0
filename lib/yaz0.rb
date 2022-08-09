require "stringio"
require "yaz0/version"
require "yaz0/yaz0"

module Yaz0
  class Stream
    def decompress(src, dst = nil)
      if dst
        raw_decompress(src, dst)
        nil
      else
        dst = StringIO.new
        raw_decompress(src, dst)
        dst.string
      end
    end

    def compress(src, dst_or_opts = nil, opts = {})
      if dst_or_opts.is_a?(Hash)
        opts = dst_or_opts
        dst = nil
      else
        dst = dst_or_opts
      end

      level = opts[:level] || 6
      size = opts[:size] || src.size

      if dst
        raw_compress(src, dst, size, level)
        nil
      else
        dst = StringIO.new
        raw_compress(src, dst, size, level)
        dst.string
      end
    end
  end

  def self.decompress(*args)
    Yaz0::Stream.new.decompress(*args)
  end

  def self.compress(*args)
    Yaz0::Stream.new.compress(*args)
  end
end
