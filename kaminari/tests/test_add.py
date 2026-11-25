import torch
from torch.testing._internal.common_utils import TestCase, run_tests


class TestKaminariAdd(TestCase):
    def test_add_matches_cpu(self):
        a = torch.tensor([1.0, 2.0, 3.0, 4.0])
        b = torch.tensor([10.0, 20.0, 30.0, 40.0])
        ta = a.to("kaminari")
        tb = b.to("kaminari")
        self.assertEqual((ta + tb).cpu(), a + b)

    def test_add_does_not_use_fallback(self):
        a = torch.ones(64, device="kaminari")
        b = torch.full((64,), 2.0, device="kaminari")
        before = torch.kaminari._C._fallback_count()
        c = a + b
        after = torch.kaminari._C._fallback_count()
        self.assertEqual(after, before)
        self.assertEqual(c.cpu(), torch.full((64,), 3.0))


if __name__ == "__main__":
    run_tests()
