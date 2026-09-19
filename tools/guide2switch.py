#!/usr/bin/env python3
"""
guide2switch — core converter for the Walkthrough Overlay (Nintendo Switch).

The overlay only gets a few megabytes of RAM, so anything heavy is done here on the PC:

  * PDF   -> page images (page_001.png, ...). The overlay treats a folder of images as
             one document and flips pages with L / R. Pages are RENDERED (not compressed)
             at `width` px, 2x the overlay's 448 px by default, so zooming 2x stays sharp.
  * image -> resized with Lanczos to at most `width` px wide; PNG stays lossless PNG,
             JPEG becomes baseline JPEG (quality 85). Smaller images are left as they are.
  * text  -> UTF-8, Unix line endings. Nothing else changes.

Double-click guide2switch.bat for the graphical tool (guide2switch_gui.py).

Command line:
    python guide2switch.py --name "Zelda TOTK" guide.pdf map.png notes.txt [-o OUT] [--width 896] [--jpeg]
    python guide2switch.py --cli            # convert everything in tools/input/, sub-folder = guide name

Requirements: Python 3. pymupdf and pillow are pip-installed automatically if missing.
"""
import argparse
import importlib
import io
import subprocess
import sys
from pathlib import Path

DEFAULT_WIDTH = 896   # 2 x the 448 px overlay width
WIDTH_PRESETS = {448: "1x - smallest files, no zoom detail", 896: "2x - recommended", 1344: "3x - big files, max zoom"}
IMAGE_EXT = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp", ".tif", ".tiff"}
TEXT_EXT = {".txt", ".md", ".log", ".nfo", ".ini"}
JPEG_QUALITY = 85

HERE = Path(__file__).resolve().parent
INPUT_DIR = HERE / "input"
OUTPUT_DIR = HERE / "guides_out"
SD_SUBDIR = Path("switch") / "WalkthroughOverlay" / "guides"

_log_fn = None  # the GUI redirects log() here


def log(msg=""):
    if _log_fn:
        _log_fn(msg)
    else:
        print(msg, flush=True)


# ------------------------------------------------------------------ dependencies

def ensure(module: str, pip_name: str):
    """Import `module`; if missing, pip-install `pip_name` with this interpreter and retry."""
    try:
        return importlib.import_module(module)
    except ImportError:
        pass
    log(f"'{pip_name}' is not installed - installing it with pip (one time)...")
    cmd = [sys.executable, "-m", "pip", "install", "--quiet", "--disable-pip-version-check", "--timeout", "20", "--retries", "1", pip_name]
    try:
        subprocess.check_call(cmd)
    except Exception:
        subprocess.check_call(cmd + ["--user"])  # some Windows installs need --user
    importlib.invalidate_caches()
    return importlib.import_module(module)


def load_fitz():
    for name in ("pymupdf", "fitz"):
        try:
            return importlib.import_module(name)
        except ImportError:
            continue
    return ensure("pymupdf", "pymupdf")


def load_pil():
    ensure("PIL", "pillow")
    from PIL import Image
    return Image


# ------------------------------------------------------------------ helpers

def kind_of(path: Path) -> str:
    ext = path.suffix.lower()
    if ext == ".pdf":
        return "pdf"
    if ext in IMAGE_EXT:
        return "image"
    if ext in TEXT_EXT:
        return "text"
    return "other"


def human(n: int) -> str:
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024 or unit == "GB":
            return f"{n:.0f} {unit}" if unit == "B" else f"{n:.1f} {unit}"
        n /= 1024.0
    return f"{n:.1f} GB"


def safe_name(name: str) -> str:
    """A guide name usable as a FAT folder name."""
    bad = '<>:"/\\|?*'
    out = "".join("_" if c in bad or ord(c) < 32 else c for c in name).strip(" .")
    return out or "Guide"


def find_sd_guides():
    """A mounted SD card that already has switch/WalkthroughOverlay/guides, or None."""
    roots = []
    if sys.platform.startswith("win"):
        import string
        roots = [Path(f"{d}:\\") for d in string.ascii_uppercase[3:]]  # D: .. Z:
    else:
        for base in ("/Volumes", "/media", "/run/media", "/mnt"):
            b = Path(base)
            if not b.is_dir():
                continue
            for p in b.iterdir():
                if p.is_dir():
                    roots.append(p)
                    if base in ("/media", "/run/media"):  # /media/<user>/<volume>
                        try:
                            roots += [q for q in p.iterdir() if q.is_dir()]
                        except OSError:
                            pass
    for r in roots:
        try:
            cand = r / SD_SUBDIR
            if cand.is_dir():
                return cand
        except OSError:
            continue
    return None


def copy_tree(src: Path, dst: Path) -> int:
    """Copy new/updated files from src to dst keeping the layout; returns the count."""
    import shutil
    copied = 0
    for f in sorted(src.rglob("*")):
        if f.is_dir():
            continue
        t = dst / f.relative_to(src)
        try:
            if t.exists() and t.stat().st_mtime >= f.stat().st_mtime and t.stat().st_size == f.stat().st_size:
                continue
        except OSError:
            pass
        t.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(f, t)
        copied += 1
    return copied


# ------------------------------------------------------------------ image maths (shared with the preview)

def prepare_image(im, width: int):
    """Flatten transparency, force RGB, resize to at most `width` px wide (Lanczos)."""
    Image = load_pil()
    if im.mode in ("RGBA", "LA", "P"):
        im = im.convert("RGBA")
        bg = Image.new("RGB", im.size, (255, 255, 255))
        bg.paste(im, mask=im.split()[-1])
        im = bg
    elif im.mode != "RGB":
        im = im.convert("RGB")
    if im.width > width:
        im = im.resize((width, max(1, round(im.height * width / im.width))), Image.LANCZOS)
    return im


def encode_image(im, jpeg: bool) -> bytes:
    """The exact bytes that would be written for `im`."""
    buf = io.BytesIO()
    if jpeg:
        im.save(buf, "JPEG", quality=JPEG_QUALITY, progressive=False, optimize=True)
    else:
        im.save(buf, "PNG", optimize=True, interlace=0)
    return buf.getvalue()


def pdf_page_count(src: Path) -> int:
    fitz = load_fitz()
    with fitz.open(str(src)) as doc:
        return doc.page_count


def render_pdf_page(src: Path, index: int, width: int):
    """Render page `index` (0-based) of a PDF to a PIL RGB image `width` px wide."""
    fitz = load_fitz()
    Image = load_pil()
    with fitz.open(str(src)) as doc:
        page = doc[index]
        zoom = width / page.rect.width if page.rect.width else 1.0
        pix = page.get_pixmap(matrix=fitz.Matrix(zoom, zoom), alpha=False)
        return Image.frombytes("RGB", (pix.width, pix.height), pix.samples)


# ------------------------------------------------------------------ jobs

class Job:
    """One source file and the folder its result goes to.
    `flat` (PDF only): write the pages directly into out_dir instead of out_dir/<pdf name>/."""

    def __init__(self, src: Path, out_dir: Path, flat: bool = False):
        self.src = Path(src)
        self.kind = kind_of(self.src)
        self.out_dir = Path(out_dir)
        self.flat = flat

    def target(self) -> Path:
        if self.kind == "pdf":
            return self.out_dir if self.flat else self.out_dir / self.src.stem
        if self.kind == "image":
            ext = ".jpg" if self.src.suffix.lower() in (".jpg", ".jpeg") else ".png"
            return self.out_dir / (self.src.stem + ext)
        if self.kind == "text":
            return self.out_dir / (self.src.stem + ".txt")
        return self.out_dir / self.src.name

    def describe(self) -> str:
        t = self.target()
        return f"{t.name}/page_001.png ..." if self.kind == "pdf" else t.name


def plan_guide(name: str, files, out_root: Path) -> list:
    """All `files` go into out_root/<name>/. A single PDF is flattened (its pages become
    the guide); several PDFs each get their own sub-folder."""
    files = [Path(f) for f in files]
    guide_dir = out_root / safe_name(name)
    pdfs = [f for f in files if kind_of(f) == "pdf"]
    others = [f for f in files if kind_of(f) in ("image", "text")]
    flat = len(pdfs) == 1 and not any(kind_of(f) == "image" for f in others)
    jobs = [Job(f, guide_dir, flat=flat) for f in pdfs]
    jobs += [Job(f, guide_dir) for f in others]
    return jobs


def plan_input_folder(input_dir: Path, out_root: Path) -> list:
    """tools/input/<Guide name>/... -> one guide per sub-folder; loose files -> guide 'Misc'."""
    jobs = []
    if not input_dir.is_dir():
        return jobs
    loose = [f for f in sorted(input_dir.iterdir()) if f.is_file() and kind_of(f) != "other"]
    if loose:
        jobs += plan_guide("Misc", loose, out_root)
    for d in sorted(p for p in input_dir.iterdir() if p.is_dir() and not p.name.startswith(".")):
        files = [f for f in sorted(d.rglob("*")) if f.is_file() and kind_of(f) != "other"]
        if files:
            jobs += plan_guide(d.name, files, out_root)
    return jobs


# ------------------------------------------------------------------ converters

def convert_pdf(job: Job, width: int, jpeg: bool, progress=None):
    fitz = load_fitz()
    src, doc_dir = job.src, job.target()
    doc_dir.mkdir(parents=True, exist_ok=True)
    with fitz.open(str(src)) as doc:
        n = doc.page_count
        digits = max(3, len(str(n)))
        log(f"PDF  {src.name}: {n} pages -> {doc_dir}")
        for i, page in enumerate(doc, start=1):
            zoom = width / page.rect.width if page.rect.width else 1.0
            pix = page.get_pixmap(matrix=fitz.Matrix(zoom, zoom), alpha=False)
            target = doc_dir / (f"page_{i:0{digits}d}." + ("jpg" if jpeg else "png"))
            if jpeg:
                pix.save(str(target), jpg_quality=JPEG_QUALITY)
            else:
                pix.save(str(target))
            if progress:
                progress(i, n)
            if i % 10 == 0 or i == n:
                log(f"     {i}/{n}")


def convert_image(job: Job, width: int):
    Image = load_pil()
    src, target = job.src, job.target()
    target.parent.mkdir(parents=True, exist_ok=True)
    with Image.open(src) as im:
        im.load()
        out = prepare_image(im, width)
        target.write_bytes(encode_image(out, target.suffix == ".jpg"))
        log(f"IMG  {src.name} -> {target.name} ({out.width}x{out.height}, {human(target.stat().st_size)})")


def decode_text(raw: bytes) -> str:
    for enc in ("utf-8-sig", "utf-8", "cp1252", "latin-1"):
        try:
            return raw.decode(enc).replace("\r\n", "\n").replace("\r", "\n")
        except UnicodeDecodeError:
            continue
    return raw.decode("latin-1", errors="replace")


def convert_text(job: Job):
    src, target = job.src, job.target()
    target.parent.mkdir(parents=True, exist_ok=True)
    text = decode_text(src.read_bytes())
    target.write_text(text, encoding="utf-8", newline="\n")
    log(f"TXT  {src.name} -> {target.name} ({text.count(chr(10)) + 1} lines)")


def run_jobs(jobs, width: int, jpeg: bool, progress=None) -> int:
    """Convert every job. `progress(done_jobs, total_jobs, page, pages)` is optional."""
    done = 0
    total = len(jobs)
    for job in jobs:
        try:
            if job.kind == "pdf":
                convert_pdf(job, width, jpeg, progress=(lambda p, n, d=done: progress(d, total, p, n)) if progress else None)
            elif job.kind == "image":
                convert_image(job, width)
            elif job.kind == "text":
                convert_text(job)
            done += 1
            if progress:
                progress(done, total, 0, 0)
        except Exception as e:  # keep going with the other files
            log(f"!! {job.src.name}: {e}")
    return done


def copy_to_sd(out_dir: Path, sd_arg: str):
    if not sd_arg or sd_arg.lower() in ("no", "none", "0"):
        return
    sd = find_sd_guides() if sd_arg == "auto" else Path(sd_arg)
    if sd and sd.is_dir() and out_dir.exists():
        n = copy_tree(out_dir, sd)
        log(f"SD card: {sd} - {n} file(s) copied.")
    elif sd_arg == "auto":
        log("No SD card with switch/WalkthroughOverlay/guides detected - copy the output folder there yourself.")
    else:
        log(f"!! SD folder not found: {sd}")


# ------------------------------------------------------------------ main

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("inputs", nargs="*", help="files of one guide (PDF / images / text)")
    ap.add_argument("--name", help="guide name = destination folder (default: name of the first file)")
    ap.add_argument("-o", "--out", default=str(OUTPUT_DIR), help="output root (default: tools/guides_out)")
    ap.add_argument("--width", type=int, default=DEFAULT_WIDTH, help=f"max page/image width in px (default {DEFAULT_WIDTH})")
    ap.add_argument("--jpeg", action="store_true", help="PDF pages as JPEG instead of PNG (smaller, for scans)")
    ap.add_argument("--sd", default="auto", help="SD guides folder to copy the result to ('auto' = detect, 'no' = never)")
    ap.add_argument("--cli", action="store_true", help="convert tools/input/ (one sub-folder per guide)")
    args = ap.parse_args()

    out_root = Path(args.out)
    if not out_root.is_absolute():
        out_root = HERE / out_root

    if args.inputs:
        files = [Path(p) for p in args.inputs]
        missing = [f for f in files if not f.exists()]
        for f in missing:
            log(f"!! not found: {f}")
        files = [f for f in files if f.exists()]
        expanded = []
        for f in files:
            expanded += [g for g in sorted(f.rglob("*")) if g.is_file()] if f.is_dir() else [f]
        name = args.name or (files[0].stem if files else "Guide")
        jobs = plan_guide(name, [f for f in expanded if kind_of(f) != "other"], out_root)
    elif args.cli:
        INPUT_DIR.mkdir(parents=True, exist_ok=True)
        jobs = plan_input_folder(INPUT_DIR, out_root)
        log(f"Scanning {INPUT_DIR}: {len(jobs)} file(s)")
    else:
        # No arguments: open the graphical tool.
        from guide2switch_gui import run_gui
        return run_gui()

    n = run_jobs(jobs, args.width, args.jpeg)
    log(f"Done: {n} file(s) converted -> {out_root}")
    copy_to_sd(out_root, args.sd)


if __name__ == "__main__":
    main()
