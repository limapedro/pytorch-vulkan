"""Device module registered as ``torch.kaminari``."""

import torch

from . import _C, _utils

_initialized = False


def device():
    """Context manager selecting the Kaminari device (only index 0 exists)."""

    class _Ctx:
        def __enter__(self):
            return self

        def __exit__(self, exc_type, exc_value, traceback):
            return False

    return _Ctx()


def is_available():
    return True


def device_count():
    return _C._get_device_count()


def current_device():
    return _C._get_device()


def set_device(index):
    if index >= 0:
        _C._set_device(index)


def init():
    _lazy_init()


def is_initialized():
    return _initialized


def _lazy_init():
    global _initialized
    if not _initialized:
        _C._init()
        _initialized = True


def _is_in_bad_fork():
    return False


_seed = -1


def manual_seed(seed):
    """Placeholder: record the seed until the Vulkan RNG lands in Phase 5."""
    global _seed
    _lazy_init()
    _seed = int(seed)
    torch.Generator().manual_seed(_seed)


def manual_seed_all(seed):
    manual_seed(seed)


def initial_seed():
    return _seed


def get_rng_state():
    return torch.get_rng_state()


def set_rng_state(state):
    torch.set_rng_state(state)


def get_amp_supported_dtype():
    return [torch.float32]


_utils = _utils
