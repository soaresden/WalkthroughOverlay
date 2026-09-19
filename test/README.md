# Native test harness

Builds the platform-independent parts (image loader, text document, file helpers)
on a Linux/macOS PC under AddressSanitizer and exercises them with generated data.

```
sudo apt install libpng-dev libjpeg-dev   # or brew install libpng jpeg
pip install pillow numpy
python3 gen_data.py
g++ -std=c++20 -O1 -g -fsanitize=address,undefined -I stub -I ../include \
    harness.cpp ../source/ImageLoader.cpp ../source/TextDocument.cpp ../source/Files.cpp \
    -lpng -ljpeg -o harness && ./harness
```
