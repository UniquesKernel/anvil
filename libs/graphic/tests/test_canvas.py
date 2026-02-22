from graphic_module import Canvas, MAX_HEIGHT, MAX_WIDTH

def test_not_implemented():
    canvas = Canvas(100, 200, ' ')
    assert canvas.height < MAX_HEIGHT
    assert canvas.width < MAX_WIDTH
    assert canvas.buffer[0] == ' '
