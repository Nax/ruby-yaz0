require_relative 'lib/yaz0/version'

Gem::Specification.new do |spec|
  spec.name          = "yaz0"
  spec.version       = Yaz0::VERSION
  spec.authors       = ["Nax"]
  spec.email         = ["max@bacoux.com"]

  spec.summary       = "Compress and decompress Yaz0 data."
  spec.homepage      = "https://github.com/Nax/ruby-yaz0"
  spec.license       = "MIT"
  spec.required_ruby_version = Gem::Requirement.new(">= 2.3.0")
  spec.metadata["source_code_uri"] = "https://github.com/Nax/ruby-yaz0"

  # Specify which files should be added to the gem when it is released.
  # The `git ls-files -z` loads the files in the RubyGem that have been added into git.
  spec.files         = Dir[
    "ext/**/*",
    "lib/**/*",
    "spec/**/*",
    "libyaz0/include/**/*",
    "libyaz0/src/libyaz0/**/*",
    "LICENSE",
    "README.md",
    "Gemfile",
    "Rakefile",
    "yaz0.gemspec",
    ".rspec"
  ]
  spec.bindir        = "exe"
  spec.executables   = spec.files.grep(%r{^exe/}) { |f| File.basename(f) }
  spec.require_paths = ["lib"]
  spec.extensions    = ["ext/yaz0/extconf.rb"]
end
