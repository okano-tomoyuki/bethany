"""wheel のタグを決める(pyproject.toml の [tool.hatch.build.targets.wheel.hooks.custom])。

beth は Python の拡張モジュールを持たず、同梱の beth.dll を ctypes で読み込むだけなので、Python の版には依存しない(py3-none)。
beth.dll は Windows x64 向けにビルドしたもので、C のランタイムにも依存しない(Windows の DLL だけを読み込む)。そのため、
python.org の Python でも MSYS2 の Python でも同じ beth.dll が動く。プラットフォームのタグは、ビルドする Python のものにする。

- 配布の wheel(package-python.sh が python.org の Python で作る)は py3-none-win_amd64 になる。
- PyPI は MSYS2 のタグ(mingw_x86_64_ucrt_gnu 等)の wheel を受け付けない。MSYS2 の pip は sdist からこのフックで wheel を作り、
  そのタグ(py3-none-mingw_x86_64_ucrt_gnu 等)で入れる(docs/adr/0055)。
- Windows x64 以外(Linux・32 ビットの Python 等)では、beth.dll が動かないためビルドを止める。
"""
import sysconfig

from hatchling.builders.hooks.plugin.interface import BuildHookInterface


def _platform_tag():
    # sysconfig.get_platform() は win-amd64・mingw_x86_64_ucrt_gnu 等。wheel のタグでは - と . を _ にする。
    return sysconfig.get_platform().replace("-", "_").replace(".", "_")


class CustomBuildHook(BuildHookInterface):
    def initialize(self, version, build_data):
        platform = _platform_tag()
        if platform != "win_amd64" and not platform.startswith("mingw_x86_64"):
            raise RuntimeError(
                f"bethany-lcl は Windows x64 の Python(python.org 版か MSYS2 版)専用です(この Python は {platform})"
            )
        build_data["pure_python"] = False
        build_data["tag"] = f"py3-none-{platform}"
