"""Helpers required by torch.load / torch.device for private-use backends."""


def _get_device_index(device, optional=False):
    if device is None and optional:
        return -1
    if isinstance(device, int):
        if device < 0 and optional:
            return -1
        if device != 0:
            raise ValueError(f"kaminari only supports device 0, got {device}")
        return device
    name = getattr(device, "type", None)
    if name in ("kaminari", "privateuse1"):
        index = getattr(device, "index", None)
        return 0 if index is None else int(index)
    if optional:
        return -1
    raise TypeError(f"expected a kaminari device, got {device!r}")
