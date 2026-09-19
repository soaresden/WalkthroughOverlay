#!/usr/bin/env python3
"""
guide2switch_gui — graphical front-end for guide2switch.

  1. Drop the files of ONE walkthrough (PDF, images, text) into the window, or use "Add files".
  2. Type the guide's name: that is the folder the Switch will show.
  3. Look at the Before / After preview at the zoom levels the overlay uses.
  4. Convert. The result lands in tools/guides_out/<name>/ and, if an SD card with
     switch/WalkthroughOverlay/guides is plugged in, on the card as well.

Nothing here needs more than Python 3 with Tk. Drag & drop uses tkinterdnd2 when it can be
installed; otherwise the buttons do the same job.
"""
import queue
import sys
import threading
from pathlib import Path

import guide2switch as core

CANVAS_W, CANVAS_H = 400, 520
ZOOMS = [("0.5x  - Switch, fit width", 0.5), ("1x", 1.0), ("2x  - Switch, zoomed in", 2.0), ("4x", 4.0)]


class PreviewSource:
    """Before / after images of one file at the current settings (cached)."""

    def __init__(self, path: Path, kind: str):
        self.path, self.kind = path, kind
        self.pages = core.pdf_page_count(path) if kind == "pdf" else 1
        self._cache = {}

    def before(self, page: int, width: int):
        key = ("before", page, width if self.kind == "pdf" else 0)
        if key not in self._cache:
            if self.kind == "pdf":
                # reference rendering with twice the detail of the output, capped
                self._cache[key] = core.render_pdf_page(self.path, page, min(width * 2, 2688))
            else:
                Image = core.load_pil()
                with Image.open(self.path) as im:
                    im.load()
                    self._cache[key] = im.convert("RGB") if im.mode != "RGB" else im.copy()
        return self._cache[key]

    def after(self, page: int, width: int, jpeg: bool):
        """(PIL image exactly as it will be written, encoded size in bytes)."""
        key = ("after", page, width, jpeg)
        if key not in self._cache:
            if self.kind == "pdf":
                im = core.render_pdf_page(self.path, page, width)
            else:
                Image = core.load_pil()
                with Image.open(self.path) as src:
                    src.load()
                    im = core.prepare_image(src, width)
            use_jpeg = jpeg if self.kind == "pdf" else self.path.suffix.lower() in (".jpg", ".jpeg")
            self._cache[key] = (im, len(core.encode_image(im, use_jpeg)), "JPEG" if use_jpeg else "PNG")
        return self._cache[key]


def run_gui():
    import tkinter as tk
    from tkinter import ttk, filedialog, messagebox
    core.load_pil()
    from PIL import Image, ImageTk

    # ---- optional drag & drop
    dnd = None
    try:
        dnd = core.ensure("tkinterdnd2", "tkinterdnd2")
    except Exception:
        dnd = None
    root = dnd.TkinterDnD.Tk() if dnd else tk.Tk()
    root.title("guide2switch - Walkthrough Overlay")
    root.geometry("1320x800")
    root.minsize(1100, 640)

    events = queue.Queue()
    state = {"files": [], "sources": {}, "jobs": [], "preview": None, "page": 0,
             "busy": False, "ox": 0.0, "oy": 0.0, "photo_b": None, "photo_a": None, "seq": 0}

    v_name = tk.StringVar(value="")
    v_width = tk.IntVar(value=core.DEFAULT_WIDTH)
    v_jpeg = tk.BooleanVar(value=False)
    v_out = tk.StringVar(value=str(core.OUTPUT_DIR))
    v_copy_sd = tk.BooleanVar(value=True)
    v_sd = tk.StringVar(value="")
    v_zoom = tk.DoubleVar(value=0.5)
    v_page = tk.IntVar(value=1)

    # ================================================================ left column
    left = ttk.Frame(root, padding=8, width=450)
    left.pack(side="left", fill="y")
    left.pack_propagate(False)

    ttk.Label(left, text="1.  Files of the walkthrough", font=("", 10, "bold")).pack(anchor="w")
    tree = ttk.Treeview(left, columns=("file", "type", "dest"), show="headings", height=9, selectmode="browse")
    for c, w, t in (("file", 170, "File"), ("type", 50, "Type"), ("dest", 200, "On the Switch")):
        tree.heading(c, text=t)
        tree.column(c, width=w, anchor="w", stretch=(c == "dest"))
    tree.pack(fill="x")
    drop_hint = ttk.Label(left, text=("Drop PDF / image / text files here, or use the buttons." if dnd
                                      else "Use the buttons to add PDF / image / text files."), foreground="#666")
    drop_hint.pack(anchor="w", pady=(2, 4))

    btns = ttk.Frame(left)
    btns.pack(fill="x")

    def add_paths(paths):
        added = 0
        for p in paths:
            p = Path(p)
            if p.is_dir():
                added += add_paths(sorted(p.rglob("*")))
                continue
            if not p.is_file() or core.kind_of(p) == "other" or p in state["files"]:
                continue
            state["files"].append(p)
            added += 1
        if added:
            if not v_name.get().strip():
                v_name.set(state["files"][0].stem)
            refresh_plan()
            if len(tree.get_children()) and not tree.selection():
                tree.selection_set(tree.get_children()[0])
        return added

    def add_files():
        paths = filedialog.askopenfilenames(title="Add files",
            filetypes=[("Guides", "*.pdf *.png *.jpg *.jpeg *.bmp *.gif *.webp *.txt *.md *.nfo *.log"), ("All files", "*.*")])
        add_paths(paths)

    def add_folder():
        d = filedialog.askdirectory(title="Add a folder")
        if d:
            add_paths([d])

    def remove_selected():
        for iid in tree.selection():
            idx = tree.index(iid)
            if 0 <= idx < len(state["files"]):
                del state["files"][idx]
        refresh_plan()

    def clear_all():
        state["files"].clear()
        refresh_plan()

    ttk.Button(btns, text="Add files...", command=add_files).pack(side="left")
    ttk.Button(btns, text="Add folder...", command=add_folder).pack(side="left", padx=4)
    ttk.Button(btns, text="Remove", command=remove_selected).pack(side="left")
    ttk.Button(btns, text="Clear", command=clear_all).pack(side="left", padx=4)

    ttk.Label(left, text="2.  Guide name  (folder shown on the Switch)", font=("", 10, "bold")).pack(anchor="w", pady=(12, 0))
    e_name = ttk.Entry(left, textvariable=v_name, font=("", 11))
    e_name.pack(fill="x")

    ttk.Label(left, text="3.  Quality", font=("", 10, "bold")).pack(anchor="w", pady=(12, 0))
    for w, desc in core.WIDTH_PRESETS.items():
        ttk.Radiobutton(left, text=f"{w} px wide   ({desc})", variable=v_width, value=w).pack(anchor="w")
    ttk.Checkbutton(left, text="PDF pages as JPEG instead of PNG (much smaller; for scans)", variable=v_jpeg).pack(anchor="w")
    ttk.Label(left, text="The overlay is 448 px wide: pages are shown 'fit width', and 896 px "
                         "means the 2x zoom on the Switch is still sharp.",
              foreground="#666", wraplength=420).pack(anchor="w")

    ttk.Label(left, text="4.  Output", font=("", 10, "bold")).pack(anchor="w", pady=(12, 0))
    outrow = ttk.Frame(left)
    outrow.pack(fill="x")
    ttk.Entry(outrow, textvariable=v_out).pack(side="left", fill="x", expand=True)

    def open_folder(p: Path):
        p.mkdir(parents=True, exist_ok=True)
        if sys.platform.startswith("win"):
            import os
            os.startfile(str(p))
        elif sys.platform == "darwin":
            import subprocess
            subprocess.Popen(["open", str(p)])
        else:
            import subprocess
            subprocess.Popen(["xdg-open", str(p)])

    ttk.Button(outrow, text="Open", command=lambda: open_folder(Path(v_out.get()))).pack(side="left", padx=(4, 0))
    sdrow = ttk.Frame(left)
    sdrow.pack(fill="x", pady=(2, 0))
    ttk.Checkbutton(sdrow, text="Copy to SD card:", variable=v_copy_sd).pack(side="left")
    sd_label = ttk.Label(sdrow, text="", foreground="#888")
    sd_label.pack(side="left", padx=4)

    def detect_sd():
        sd = core.find_sd_guides()
        v_sd.set(str(sd) if sd else "")
        sd_label.config(text=str(sd) if sd else "not detected", foreground="#1a7f37" if sd else "#888")

    ttk.Button(sdrow, text="Detect", command=detect_sd).pack(side="right")

    progress = ttk.Progressbar(left, mode="determinate", maximum=1000)
    progress.pack(fill="x", pady=(12, 2))
    btn_convert = ttk.Button(left, text="Convert", command=lambda: convert())
    btn_convert.pack(fill="x")

    logbox = tk.Text(left, height=7, state="disabled", wrap="word", background="#f6f6f6", font=("", 9))
    logbox.pack(fill="both", expand=True, pady=(8, 0))

    def append_log(msg):
        logbox.configure(state="normal")
        logbox.insert("end", msg + "\n")
        logbox.see("end")
        logbox.configure(state="disabled")

    # ================================================================ right column: preview
    right = ttk.Frame(root, padding=8)
    right.pack(side="left", fill="both", expand=True)

    head = ttk.Frame(right)
    head.pack(fill="x")
    ttk.Label(head, text="Before / after preview", font=("", 10, "bold")).pack(side="left")
    ttk.Label(head, text="   Zoom:").pack(side="left")
    for label, z in ZOOMS:
        ttk.Radiobutton(head, text=label, variable=v_zoom, value=z, command=lambda: schedule_render()).pack(side="left", padx=2)
    page_frame = ttk.Frame(head)
    page_frame.pack(side="right")
    ttk.Label(page_frame, text="Page").pack(side="left")
    page_spin = ttk.Spinbox(page_frame, from_=1, to=1, width=5, textvariable=v_page, command=lambda: on_page())
    page_spin.pack(side="left", padx=4)
    page_total = ttk.Label(page_frame, text="/ 1")
    page_total.pack(side="left")

    panels = ttk.Frame(right)
    panels.pack(fill="both", expand=True, pady=(6, 0))
    lbl_before = ttk.Label(panels, text="Before", anchor="w", wraplength=CANVAS_W)
    lbl_after = ttk.Label(panels, text="After", anchor="w", wraplength=CANVAS_W)
    lbl_before.grid(row=0, column=0, sticky="w")
    lbl_after.grid(row=0, column=1, sticky="w", padx=(8, 0))
    cv_before = tk.Canvas(panels, width=CANVAS_W, height=CANVAS_H, background="#333", highlightthickness=0)
    cv_after = tk.Canvas(panels, width=CANVAS_W, height=CANVAS_H, background="#333", highlightthickness=0)
    cv_before.grid(row=1, column=0, sticky="nsew")
    cv_after.grid(row=1, column=1, sticky="nsew", padx=(8, 0))
    vbar = ttk.Scrollbar(panels, orient="vertical")
    hbar = ttk.Scrollbar(panels, orient="horizontal")
    vbar.grid(row=1, column=2, sticky="ns")
    hbar.grid(row=2, column=0, columnspan=2, sticky="ew")
    panels.columnconfigure(0, weight=1, uniform="panel")
    panels.columnconfigure(1, weight=1, uniform="panel")
    panels.rowconfigure(1, weight=1)
    text_preview = tk.Text(panels, wrap="none", font=("Courier New", 10), state="disabled")
    totals = ttk.Label(right, text="", foreground="#444")
    totals.pack(anchor="w", pady=(6, 0))
    hint = ttk.Label(right, text="Drag inside a panel or use the scrollbars to pan; both panels show the same area. "
                                 "'After' is the exact file that will be written; 'Before' is the source at its own resolution.",
                     foreground="#666", wraplength=820)
    hint.pack(anchor="w")

    # ---------------------------------------------------------------- preview maths
    def canvas_size():
        w = max(50, cv_after.winfo_width() or CANVAS_W)
        h = max(50, cv_after.winfo_height() or CANVAS_H)
        return w, h

    def virtual_size():
        p = state["preview"]
        if not p:
            return 1, 1
        im, _, _ = p["after"]
        z = v_zoom.get()
        return max(1, int(im.width * z)), max(1, int(im.height * z))

    def clamp_offsets():
        vw, vh = virtual_size()
        cw, ch = canvas_size()
        state["ox"] = max(0.0, min(state["ox"], vw - cw)) if vw > cw else 0.0
        state["oy"] = max(0.0, min(state["oy"], vh - ch)) if vh > ch else 0.0

    def update_scrollbars():
        vw, vh = virtual_size()
        cw, ch = canvas_size()
        hbar.set(state["ox"] / vw, min(1.0, (state["ox"] + cw) / vw))
        vbar.set(state["oy"] / vh, min(1.0, (state["oy"] + ch) / vh))

    def on_scroll(axis, *args):
        vw, vh = virtual_size()
        cw, ch = canvas_size()
        size = vw if axis == "x" else vh
        view = cw if axis == "x" else ch
        key = "ox" if axis == "x" else "oy"
        if args[0] == "moveto":
            state[key] = float(args[1]) * size
        elif args[0] == "scroll":
            step = view * 0.9 if args[2] == "pages" else 40
            state[key] += int(args[1]) * step
        clamp_offsets()
        render_now()

    hbar.config(command=lambda *a: on_scroll("x", *a))
    vbar.config(command=lambda *a: on_scroll("y", *a))

    def render_now():
        """Draw the visible region of both panels for the current zoom / offsets."""
        p = state["preview"]
        for cv in (cv_before, cv_after):
            cv.delete("all")
        if not p:
            return
        after_im, _, _ = p["after"]
        before_im = p["before"]
        z = v_zoom.get()
        cw, ch = canvas_size()
        clamp_offsets()
        vw, vh = virtual_size()
        draw_w, draw_h = min(cw, vw), min(ch, vh)
        # region in after-pixel coordinates
        ax0, ay0 = state["ox"] / z, state["oy"] / z
        ax1, ay1 = ax0 + draw_w / z, ay0 + draw_h / z
        ax1, ay1 = min(ax1, after_im.width), min(ay1, after_im.height)
        if ax1 <= ax0 or ay1 <= ay0:
            return
        resample_up = Image.NEAREST if z >= 1 else Image.LANCZOS
        crop_a = after_im.crop((int(ax0), int(ay0), int(ax1 + 0.999), int(ay1 + 0.999)))
        img_a = crop_a.resize((draw_w, draw_h), resample_up)
        r = before_im.width / after_im.width
        crop_b = before_im.crop((int(ax0 * r), int(ay0 * r), int(ax1 * r + 0.999), int(ay1 * r + 0.999)))
        # LANCZOS when shrinking the source, NEAREST when the source itself is being magnified
        img_b = crop_b.resize((draw_w, draw_h), Image.LANCZOS if crop_b.width >= draw_w else Image.NEAREST)
        state["photo_b"] = ImageTk.PhotoImage(img_b)
        state["photo_a"] = ImageTk.PhotoImage(img_a)
        x = (cw - draw_w) // 2
        y = (ch - draw_h) // 2 if vh < ch else 0
        cv_before.create_image(x, y, anchor="nw", image=state["photo_b"])
        cv_after.create_image(x, y, anchor="nw", image=state["photo_a"])
        update_scrollbars()

    def schedule_render():
        state["ox"] = state["oy"] = 0.0
        render_now()

    # drag to pan + mouse wheel
    drag = {"x": 0, "y": 0}

    def on_press(e):
        drag["x"], drag["y"] = e.x, e.y

    def on_drag(e):
        state["ox"] -= e.x - drag["x"]
        state["oy"] -= e.y - drag["y"]
        drag["x"], drag["y"] = e.x, e.y
        clamp_offsets()
        render_now()

    def on_wheel(e):
        delta = -1 if (e.delta > 0 or getattr(e, "num", 0) == 4) else 1
        key = "ox" if (e.state & 0x1) else "oy"  # shift = horizontal
        state[key] += delta * 60
        clamp_offsets()
        render_now()

    for cv in (cv_before, cv_after):
        cv.bind("<ButtonPress-1>", on_press)
        cv.bind("<B1-Motion>", on_drag)
        cv.bind("<MouseWheel>", on_wheel)
        cv.bind("<Button-4>", on_wheel)
        cv.bind("<Button-5>", on_wheel)
        cv.bind("<Configure>", lambda e: render_now())

    # ---------------------------------------------------------------- preview loading (worker thread)
    def load_preview_async():
        sel = tree.selection()
        if not sel or state["busy"]:
            return
        idx = tree.index(sel[0])
        if idx >= len(state["files"]):
            return
        path = state["files"][idx]
        kind = core.kind_of(path)
        state["seq"] += 1
        seq = state["seq"]
        width, jpeg = v_width.get(), v_jpeg.get()

        if kind == "text":
            show_text_preview(path)
            return
        show_image_panels()
        lbl_before.config(text="Before - loading...")
        lbl_after.config(text="After - loading...")
        page = max(0, v_page.get() - 1)

        def worker():
            try:
                src = state["sources"].get(path)
                if src is None:
                    src = PreviewSource(path, kind)
                    state["sources"][path] = src
                pg = min(page, src.pages - 1)
                before = src.before(pg, width)
                after = src.after(pg, width, jpeg)
                events.put(("preview", seq, path, src, pg, before, after))
            except Exception as e:
                events.put(("preview_error", seq, path, str(e)))

        threading.Thread(target=worker, daemon=True).start()

    def show_image_panels():
        text_preview.grid_forget()
        cv_before.grid(row=1, column=0, sticky="nsew")
        cv_after.grid(row=1, column=1, sticky="nsew", padx=(8, 0))
        vbar.grid(row=1, column=2, sticky="ns")
        hbar.grid(row=2, column=0, columnspan=2, sticky="ew")

    def show_text_preview(path: Path):
        cv_before.grid_forget()
        cv_after.grid_forget()
        vbar.grid_forget()
        hbar.grid_forget()
        text_preview.grid(row=1, column=0, columnspan=3, sticky="nsew")
        raw = path.read_bytes()
        text = core.decode_text(raw)
        lines = text.split("\n")
        text_preview.configure(state="normal")
        text_preview.delete("1.0", "end")
        text_preview.insert("end", "\n".join(lines[:300]) + ("\n[...]" if len(lines) > 300 else ""))
        text_preview.configure(state="disabled")
        lbl_before.config(text=f"Before - {path.name}, {core.human(len(raw))}")
        lbl_after.config(text=f"After - same text, UTF-8, {len(lines)} lines (only the encoding changes)")
        page_total.config(text="/ 1")
        page_spin.config(to=1)
        totals.config(text="")
        state["preview"] = None

    def apply_preview(seq, path, src, pg, before, after):
        if seq != state["seq"]:
            return
        after_im, after_bytes, fmt = after
        state["preview"] = {"before": before, "after": after, "path": path}
        page_spin.config(to=src.pages)
        page_total.config(text=f"/ {src.pages}")
        if v_page.get() != pg + 1:
            v_page.set(pg + 1)
        src_size = path.stat().st_size
        if src.kind == "pdf":
            lbl_before.config(text=f"Before - PDF page {pg + 1}, rendered from the vector page ({before.width}x{before.height} shown)")
            lbl_after.config(text=f"After - {after_im.width}x{after_im.height} px {fmt}, {core.human(after_bytes)}")
            totals.config(text=f"Whole guide: {src.pages} pages x ~{core.human(after_bytes)} = ~{core.human(after_bytes * src.pages)}"
                               f"   (source PDF: {core.human(src_size)})")
        else:
            lbl_before.config(text=f"Before - {before.width}x{before.height} px, {core.human(src_size)} ({path.suffix.upper().lstrip('.')})")
            change = "unchanged size" if before.width <= after_im.width else f"resized from {before.width} px wide"
            lbl_after.config(text=f"After - {after_im.width}x{after_im.height} px {fmt}, {core.human(after_bytes)} ({change})")
            totals.config(text="")
        schedule_render()

    def on_page():
        load_preview_async()

    tree.bind("<<TreeviewSelect>>", lambda e: (v_page.set(1), load_preview_async()))
    for var in (v_width, v_jpeg):
        var.trace_add("write", lambda *_: (refresh_plan(), load_preview_async()))
    v_name.trace_add("write", lambda *_: refresh_plan())

    # ---------------------------------------------------------------- plan / convert
    def refresh_plan():
        name = v_name.get().strip() or "Guide"
        out_root = Path(v_out.get())
        state["jobs"] = core.plan_guide(name, state["files"], out_root)
        by_src = {j.src: j for j in state["jobs"]}
        sel_idx = tree.index(tree.selection()[0]) if tree.selection() else None
        for row in tree.get_children():
            tree.delete(row)
        guide = core.safe_name(name)
        for f in state["files"]:
            j = by_src.get(f)
            dest = f"{guide}/{j.describe()}" if j else "?"
            tree.insert("", "end", values=(f.name, j.kind if j else "?", dest))
        kids = tree.get_children()
        if kids and sel_idx is not None and sel_idx < len(kids):
            tree.selection_set(kids[sel_idx])

    def convert():
        if not state["jobs"]:
            messagebox.showinfo("guide2switch", "Add the files of a walkthrough first.")
            return
        if not v_name.get().strip():
            messagebox.showinfo("guide2switch", "Give the guide a name.")
            e_name.focus_set()
            return
        state["busy"] = True
        btn_convert.config(state="disabled")
        progress["value"] = 0
        jobs = list(state["jobs"])
        width, jpeg = v_width.get(), v_jpeg.get()
        out_root = Path(v_out.get())
        sd_target = (v_sd.get() or "auto") if v_copy_sd.get() else "no"
        guide_dir = jobs[0].out_dir

        def worker():
            core._log_fn = lambda m: events.put(("log", m))
            try:
                def prog(done, total, page, pages):
                    frac = (done + (page / pages if pages else 0)) / max(1, total)
                    events.put(("progress", frac))
                n = core.run_jobs(jobs, width, jpeg, progress=prog)
                core.log(f"Done: {n} file(s) -> {guide_dir}")
                core.copy_to_sd(out_root, sd_target)
            except Exception as e:
                core.log(f"!! {e}")
            finally:
                core._log_fn = None
                events.put(("done", None))

        threading.Thread(target=worker, daemon=True).start()

    # ---------------------------------------------------------------- event pump (Tk is only touched here)
    def poll():
        try:
            while True:
                ev = events.get_nowait()
                if ev[0] == "log":
                    append_log(ev[1])
                elif ev[0] == "progress":
                    progress["value"] = int(ev[1] * 1000)
                elif ev[0] == "done":
                    progress["value"] = 1000
                    state["busy"] = False
                    btn_convert.config(state="normal")
                elif ev[0] == "preview":
                    apply_preview(*ev[1:])
                elif ev[0] == "preview_error":
                    if ev[1] == state["seq"]:
                        lbl_before.config(text=f"Before - {ev[2].name}")
                        lbl_after.config(text=f"After - cannot preview: {ev[3]}")
        except queue.Empty:
            pass
        root.after(80, poll)

    # ---------------------------------------------------------------- drag & drop
    if dnd:
        def on_drop(event):
            add_paths(root.tk.splitlist(event.data))
        try:
            for w in (tree, left, cv_before, cv_after):
                w.drop_target_register(dnd.DND_FILES)
                w.dnd_bind("<<Drop>>", on_drop)
        except Exception:
            drop_hint.config(text="Use the buttons to add PDF / image / text files.")

    # files given on the command line (drag & drop onto the .bat)
    add_paths([a for a in sys.argv[1:] if not a.startswith("-")])
    detect_sd()
    poll()
    root.mainloop()


if __name__ == "__main__":
    run_gui()
