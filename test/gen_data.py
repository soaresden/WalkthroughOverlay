"""Generates the sample files used by harness.cpp into ./data and ./out."""
import os, random
from PIL import Image
os.makedirs('data', exist_ok=True); os.makedirs('out', exist_ok=True)
os.chdir('data')
w, h = 1300, 900
im = Image.new('RGB', (w, h)); px = im.load()
for y in range(h):
    for x in range(w):
        px[x, y] = (x * 255 // w, y * 255 // h, ((x // 50 + y // 50) % 2) * 255)
im.save('t_rgb.png')
im.convert('P', palette=Image.ADAPTIVE, colors=64).save('t_pal.png')
im.save('t_inter.png', interlace=1)
im.save('t_base.jpg', quality=85)
im.save('t_prog.jpg', quality=85, progressive=True)
big = im.resize((4000, 3000)); big.save('t_big.jpg', quality=80); big.save('t_big.png')
try:
    import numpy as np
    Image.fromarray((np.random.rand(300, 200) * 65535).astype('uint16')).save('t_gray16.png')
except ImportError:
    im.convert('L').save('t_gray16.png')
open('t_bad.png', 'wb').write(b'\x89PNG\r\n\x1a\n' + b'garbage' * 100)
lines = [f"Line {i}: " + ("word " * random.randint(0, 30)).strip() + ("\tTAB" if i % 7 == 0 else "") for i in range(7000)]
open('guide.txt', 'w').write("\r\n".join(lines) + "\r\n")
open('latin1.txt', 'wb').write("Pok\xe9mon caf\xe9 \xe0 la carte\n".encode('latin-1'))
open('empty.txt', 'wb').write(b'')
open('nonl.txt', 'wb').write(b'first\nsecond no newline')
print("data ready")
