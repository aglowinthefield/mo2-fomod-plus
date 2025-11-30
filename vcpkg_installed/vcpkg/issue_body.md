Package: mo2-uibase:x64-windows@2.6.0-dev.5

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.44.35221.0
-    vcpkg-tool version: 2025-07-21-d4b65a2b83ae6c3526acd1c6f3b51aff2a884533
    vcpkg-scripts version: b1b19307e2 2025-08-30 (3 months ago)

**To Reproduce**

`vcpkg install `

**Failure logs**

```
Downloading https://github.com/ModOrganizer2/modorganizer-uibase/releases/download/v2.6.0-dev.5/uibase_v2.6.0-dev.5.7z -> uibase_2.6.0-dev.5.7z
error: https://github.com/ModOrganizer2/modorganizer-uibase/releases/download/v2.6.0-dev.5/uibase_v2.6.0-dev.5.7z: failed: status code 404
note: If you are using a proxy, please ensure your proxy settings are correct.
Possible causes are:
1. You are actually using an HTTP proxy, but setting HTTPS_PROXY variable to `https//address:port`.
This is not correct, because `https://` prefix claims the proxy is an HTTPS proxy, while your proxy (v2ray, shadowsocksr, etc...) is an HTTP proxy.
Try setting `http://address:port` to both HTTP_PROXY and HTTPS_PROXY instead.
2. If you are using Windows, vcpkg will automatically use your Windows IE Proxy Settings set by your proxy software. See: https://github.com/microsoft/vcpkg-tool/pull/77
The value set by your proxy might be wrong, or have same `https://` prefix issue.
3. Your proxy's remote server is our of service.
If you believe this is not a temporary download server failure and vcpkg needs to be changed to download this file from a different location, please submit an issue to https://github.com/Microsoft/vcpkg/issues
CMake Error at scripts/cmake/vcpkg_download_distfile.cmake:136 (message):
  Download failed, halting portfile.
Call Stack (most recent call first):
  C:/Users/clearing/AppData/Local/vcpkg/registries/git-trees/2e4c2900acf97e9460ee2a532978735e7be644b1/portfile.cmake:8 (vcpkg_download_distfile)
  scripts/ports.cmake:206 (include)



```

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "name": "mo2-fomod-plus",
  "version-string": "0.0.0",
  "dependencies": [
    "7zip",
    "mo2-cmake",
    "mo2-archive",
    "mo2-uibase"
  ]
}

```
</details>
