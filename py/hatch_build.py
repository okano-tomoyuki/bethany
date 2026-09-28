"""wheel のタグを py3-none-win_amd64 にする(pyproject.toml の [tool.hatch.build.targets.wheel.hooks.custom])。

beth は Python の拡張モジュールを持たず、同梱の beth.dll を ctypes で読み込むだけなので、Python の版には依存しない(py3-none)。
beth.dll は Windows x64 向けにビルドしたものなので、プラットフォームは win_amd64 に限る。
"""
from hatchling.builders.hooks.plugin.interface import BuildHookInterface


class CustomBuildHook(BuildHookInterface):
    def initialize(self, version, build_data):
        build_data["pure_python"] = False
        build_data["tag"] = "py3-none-win_amd64"
