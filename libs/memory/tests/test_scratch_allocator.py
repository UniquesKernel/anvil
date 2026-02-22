from anvil_memory import scratch_allocator_create, scratch_allocator_destroy, Error

def test_not_implemented():
    err, allocator = scratch_allocator_create(1024,8)
    assert err == Error.OK
    assert allocator is not None

    err2 = scratch_allocator_destroy(allocator)
    assert err2 == Error.OK

    err3 = scratch_allocator_destroy(allocator)
    assert err3 == Error.NULL_PARAMETER
