"""Kaminari: a Vulkan compute backend for PyTorch.

Importing this module registers `kaminari` as a PrivateUse1 device. Once installed,
PyTorch's `torch.backends` autoload entry point performs the same registration on
`import torch`, so user scripts only need to change the device string.
"""

import torch

from . import _C, device_module

torch.utils.rename_privateuse1_backend("kaminari")
torch._register_device_module("kaminari", device_module)
torch.utils.generate_methods_for_privateuse1_backend(for_storage=True)
device_module._lazy_init()


def _autoload():
    """Entry point hook for torch.backends; registration happens on import."""
    return None
