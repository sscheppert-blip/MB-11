#!/usr/bin/env python3
"""Render docs/Schnellstart.md as a printable A4 PDF.

    python3 tools/make_quickstart_pdf.py [input.md] [output.pdf]

Needs reportlab (pip install reportlab) and the DejaVu fonts (bundled with most
Linux distributions and many Windows/macOS setups; https://dejavu-fonts.github.io).
Only the Markdown the quick start uses is supported: headings, paragraphs,
bold/italic/code, lists, tables, code blocks and rules.
"""

import os
import re
import sys
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (KeepTogether, ListFlowable, ListItem, Paragraph, Preformatted,
                                SimpleDocTemplate, Spacer, Table, TableStyle)
from reportlab.platypus.flowables import HRFlowable

ROOT = Path(__file__).resolve().parent.parent
ACCENT = colors.HexColor("#b3261e")   # the red of the panel's second-row labels
INK = colors.HexColor("#1f2328")
MUTED = colors.HexColor("#5b636b")
RULE = colors.HexColor("#d0d4d9")
TABLE_HEAD = colors.HexColor("#f1e4e2")
CODE_BG = colors.HexColor("#f4f5f7")

FONTS = {
    "Sans": "DejaVuSans.ttf",
    "Sans-Bold": "DejaVuSans-Bold.ttf",
    "Sans-Oblique": "DejaVuSans-Oblique.ttf",
    "Sans-BoldOblique": "DejaVuSans-BoldOblique.ttf",
    "Mono": "DejaVuSansMono.ttf",
    "Mono-Bold": "DejaVuSansMono-Bold.ttf",
}


def font_dirs():
    dirs = [Path(os.environ.get("WINDIR", "C:/Windows")) / "Fonts"]
    if os.environ.get("LOCALAPPDATA"):
        dirs.append(Path(os.environ["LOCALAPPDATA"]) / "Microsoft" / "Windows" / "Fonts")
    dirs += [Path.home() / "Library" / "Fonts", Path("/Library/Fonts"),
             Path.home() / ".local" / "share" / "fonts", Path("/usr/share/fonts"), Path("/usr/local/share/fonts")]
    return [d for d in dirs if d.is_dir()]


def register_fonts():
    for name, file in FONTS.items():
        found = next((p for d in font_dirs() for p in d.rglob(file)), None)
        if found is None:
            sys.exit(f"Font {file} not found – install the DejaVu fonts.")
        pdfmetrics.registerFont(TTFont(name, str(found)))
    pdfmetrics.registerFontFamily("Sans", normal="Sans", bold="Sans-Bold",
                                  italic="Sans-Oblique", boldItalic="Sans-BoldOblique")
    pdfmetrics.registerFontFamily("Mono", normal="Mono", bold="Mono-Bold", italic="Mono", boldItalic="Mono-Bold")


def styles():
    base = ParagraphStyle("body", fontName="Sans", fontSize=9.5, leading=13.5, textColor=INK, alignment=TA_LEFT)
    return {
        "body": base,
        "lead": ParagraphStyle("lead", parent=base, fontSize=11, leading=15.5, textColor=MUTED, spaceAfter=4 * mm),
        "title": ParagraphStyle("title", parent=base, fontName="Sans-Bold", fontSize=30, leading=34, textColor=INK),
        "subtitle": ParagraphStyle("subtitle", parent=base, fontSize=15, leading=20, textColor=ACCENT, spaceAfter=3 * mm),
        "h2": ParagraphStyle("h2", parent=base, fontName="Sans-Bold", fontSize=14, leading=18, textColor=ACCENT,
                             spaceBefore=6 * mm, spaceAfter=2.5 * mm, keepWithNext=1),
        "h3": ParagraphStyle("h3", parent=base, fontName="Sans-Bold", fontSize=11, leading=15,
                             spaceBefore=3.5 * mm, spaceAfter=1.5 * mm, keepWithNext=1),
        "h4": ParagraphStyle("h4", parent=base, fontName="Sans-Bold", fontSize=9.5, leading=13.5,
                             spaceBefore=3 * mm, spaceAfter=0.5 * mm, keepWithNext=1),
        "cell": ParagraphStyle("cell", parent=base, fontSize=8.8, leading=12),
        "cellHead": ParagraphStyle("cellHead", parent=base, fontName="Sans-Bold", fontSize=8.8, leading=12),
        "code": ParagraphStyle("code", parent=base, fontName="Mono", fontSize=8, leading=10.5),
    }


def inline(text):
    """Markdown inline markup → reportlab mini-HTML."""
    parts = re.split(r"(`[^`]+`)", text)
    out = []
    for part in parts:
        if part.startswith("`") and part.endswith("`") and len(part) > 1:
            code = part[1:-1].replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
            out.append(f'<font name="Mono" color="#7a1c15">{code}</font>')
            continue
        part = part.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        part = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", part)
        part = re.sub(r"(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])", r"<i>\1</i>", part)
        out.append(part)
    return "".join(out)


def table(rows, st, width):
    cells = [[c.strip() for c in row.strip().strip("|").split("|")] for row in rows]
    header, body = cells[0], [r for r in cells[1:] if not all(re.fullmatch(r":?-{3,}:?", c) for c in r)]
    columns = len(header)

    # Every column gets at least its longest word; the rest of the width goes to the
    # columns whose text would otherwise wrap, in proportion to how much they need.
    padding = 10
    def plain(text):
        return re.sub(r"[*`]", "", text)
    def word_width(text):
        return max((pdfmetrics.stringWidth(w, "Sans-Bold", 8.8) for w in plain(text).split()), default=0)
    def full_width(text):
        return pdfmetrics.stringWidth(plain(text), "Sans-Bold", 8.8)
    column = lambda i: [r[i] if i < len(r) else "" for r in [header] + body]
    minimum = [max(word_width(t) for t in column(i)) + padding for i in range(columns)]
    desired = [max(full_width(t) for t in column(i)) + padding for i in range(columns)]
    if sum(desired) <= width:
        spare = width - sum(desired)
        widths = [d + spare * d / sum(desired) for d in desired]
    else:
        extra = [d - m for d, m in zip(desired, minimum)]
        spare = max(0.0, width - sum(minimum))
        widths = [m + spare * e / (sum(extra) or 1) for m, e in zip(minimum, extra)]

    data = [[Paragraph(inline(c), st["cellHead"]) for c in header]]
    data += [[Paragraph(inline(r[i]) if i < len(r) else "", st["cell"]) for i in range(columns)] for r in body]
    t = Table(data, colWidths=widths, repeatRows=1, hAlign="LEFT")
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), TABLE_HEAD),
        ("LINEBELOW", (0, 0), (-1, 0), 0.8, ACCENT),
        ("LINEBELOW", (0, 1), (-1, -1), 0.4, RULE),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("TOPPADDING", (0, 0), (-1, -1), 3.5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 3.5),
        ("LEFTPADDING", (0, 0), (-1, -1), 5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 5),
    ]))
    # Short tables stay on one page.
    return [KeepTogether([t]) if len(data) <= 12 else t, Spacer(1, 2 * mm)]


def code_block(lines, st):
    pre = Preformatted("\n".join(lines), st["code"])
    t = Table([[pre]], hAlign="LEFT")
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), CODE_BG),
        ("BOX", (0, 0), (-1, -1), 0.4, RULE),
        ("TOPPADDING", (0, 0), (-1, -1), 6), ("BOTTOMPADDING", (0, 0), (-1, -1), 6),
        ("LEFTPADDING", (0, 0), (-1, -1), 8), ("RIGHTPADDING", (0, 0), (-1, -1), 8),
    ]))
    return [Spacer(1, 1.5 * mm), t, Spacer(1, 2.5 * mm)]


def bullet_list(items, st, ordered):
    flow = ListFlowable(
        [ListItem(Paragraph(inline(i), st["body"]), leftIndent=13) for i in items],
        bulletType="1" if ordered else "bullet", start="1" if ordered else "•",
        bulletFontName="Sans-Bold", bulletFontSize=9,
        bulletColor=ACCENT, leftIndent=13, bulletDedent=11)
    return [flow, Spacer(1, 2 * mm)]


def paragraph(lines, st):
    # A paragraph that opens with a line that is entirely bold is a small heading
    # ("**Sound speichern**" followed by its text).
    if len(lines) > 0 and re.fullmatch(r"\*\*[^*]+\*\*", lines[0].strip()):
        heading = Paragraph(inline(lines[0].strip()[2:-2]), st["h4"])
        rest = [Paragraph(inline(" ".join(lines[1:])), st["body"])] if lines[1:] else []
        return [heading] + rest + [Spacer(1, 1 * mm)]
    text = " ".join(lines)
    # Text that introduces a table or list ("…teilen – wie der Speicher im Gerät:") stays with it.
    if text.endswith(":"):
        intro = Paragraph(inline(text), ParagraphStyle("intro", parent=st["body"], spaceAfter=2 * mm))
        intro.introduces_next = True
        return [intro]
    return [Paragraph(inline(text), st["body"]), Spacer(1, 2 * mm)]


def build(md_path, pdf_path):
    register_fonts()
    st = styles()
    margin = 18 * mm
    width = A4[0] - 2 * margin - 12  # frame padding of 6 pt on each side
    lines = md_path.read_text(encoding="utf-8").splitlines()

    story, i = [], 0
    title_done = False
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        if not stripped:
            i += 1
        elif stripped.startswith("```"):
            block = []
            i += 1
            while i < len(lines) and not lines[i].strip().startswith("```"):
                block.append(lines[i])
                i += 1
            story += code_block(block, st)
            i += 1
        elif stripped.startswith("# "):
            name, _, sub = stripped[2:].partition("–")
            story += [Paragraph(inline(name.strip()), st["title"])]
            if sub.strip():
                story += [Paragraph(inline(sub.strip()), st["subtitle"])]
            story += [HRFlowable(width="100%", thickness=2, color=ACCENT, spaceBefore=1 * mm, spaceAfter=5 * mm)]
            title_done = True
            i += 1
        elif stripped.startswith("### "):
            story.append(Paragraph(inline(stripped[4:]), st["h3"]))
            i += 1
        elif stripped.startswith("## "):
            story.append(Paragraph(inline(stripped[3:]), st["h2"]))
            i += 1
        elif re.fullmatch(r"-{3,}", stripped):
            story.append(HRFlowable(width="100%", thickness=0.5, color=RULE, spaceBefore=2 * mm, spaceAfter=0))
            i += 1
        elif stripped.startswith("|"):
            rows = []
            while i < len(lines) and lines[i].strip().startswith("|"):
                rows.append(lines[i])
                i += 1
            story += table(rows, st, width)
        elif re.match(r"(\d+\.|-)\s", stripped):
            ordered = stripped[0].isdigit()
            items = []
            while i < len(lines) and re.match(r"(\d+\.|-)\s", lines[i].strip()):
                items.append(re.sub(r"^(\d+\.|-)\s+", "", lines[i].strip()))
                i += 1
            story += bullet_list(items, st, ordered)
        else:
            block = []
            while i < len(lines) and lines[i].strip() and not re.match(r"(#|\||```|-{3,}$|\d+\.\s|-\s)", lines[i].strip()):
                block.append(lines[i].strip())
                i += 1
            if not block:
                block, i = [stripped], i + 1
            # The first paragraph after the title is the lead text.
            if title_done and story and isinstance(story[-1], HRFlowable) and len(story) <= 3:
                story += [Paragraph(inline(" ".join(block)), st["lead"])]
            else:
                story += paragraph(block, st)

    # Headings and text introducing a table or list stay on the same page as the
    # block that follows them (reportlab's keepWithNext does not cover tables).
    def sticky(f):
        return getattr(f, "introduces_next", False) or (isinstance(f, Paragraph) and f.style.name in ("h2", "h3", "h4"))

    grouped, pending = [], []
    for flowable in story:
        if sticky(flowable):
            pending.append(flowable)
            continue
        if pending:
            content = flowable._content if isinstance(flowable, KeepTogether) else [flowable]
            flowable = KeepTogether(pending + list(content))
            pending = []
        grouped.append(flowable)
    grouped += pending
    story = grouped

    def decorate(canvas, doc):
        canvas.saveState()
        canvas.setFont("Sans", 7.5)
        canvas.setFillColor(MUTED)
        canvas.drawString(margin, 10 * mm, "MB-11 – Schnellstart")
        canvas.drawRightString(A4[0] - margin, 10 * mm, f"Seite {doc.page}")
        canvas.setStrokeColor(RULE)
        canvas.setLineWidth(0.4)
        canvas.line(margin, 13 * mm, A4[0] - margin, 13 * mm)
        canvas.restoreState()

    doc = SimpleDocTemplate(str(pdf_path), pagesize=A4, leftMargin=margin, rightMargin=margin,
                            topMargin=16 * mm, bottomMargin=20 * mm,
                            title="MB-11 – Schnellstart", author="MB-11", subject="Schnellstart")
    doc.build(story, onFirstPage=decorate, onLaterPages=decorate)


if __name__ == "__main__":
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "docs" / "Schnellstart.md"
    target = Path(sys.argv[2]) if len(sys.argv) > 2 else source.with_suffix(".pdf")
    build(source, target)
    print(f"wrote {target}")
