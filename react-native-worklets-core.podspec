require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))
folly_compiler_flags = '-DFOLLY_NO_CONFIG -DFOLLY_MOBILE=1 -DFOLLY_USE_LIBCPP=1 -Wno-comma -Wno-shorten-64-to-32'

Pod::Spec.new do |s|
  s.name         = "react-native-worklets-core"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => "11.0" }
  s.source       = { :git => "https://github.com/margelo/react-native-worklets-core.git", :tag => "#{s.version}" }

  s.source_files = "ios/**/*.{h,m,mm}", "cpp/**/*.{h,cpp}"
  s.pod_target_xcconfig = {
    "DEFINES_MODULE" => "YES",
  }

  install_modules_dependencies(s)
end
