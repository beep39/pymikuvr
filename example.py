render.shadows.size = 12

room = shape.box(-10, -4, -8, s=2, t=2, normalize_tc=False)
room.texture = texture("prototype.png")
room.pos.y = 2

sl = shape.sphere(0.05)
sl.color.set(1, 0, 0, 0.5)
player.left_hand.add_child(sl, True)

sr = shape.sphere(0.05)
sr.color.set(0, 1, 0, 0.5)
sr.set_parent(player.right_hand)

player.locomotion.walk(3, player.left_ctrl.axis)
