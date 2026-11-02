#!/usr/bin/env python3
"""Generate WsQ dark-theme icons for Avidemux (overwrites pics/*.png at 1x and @2x)."""
import sys, os
import cairosvg

OUT = sys.argv[1]
FG = "#cdd5e0"
ACC = "#4a9bff"
RED = "#ff7b7b"

def svg(body, w=24, h=24):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}" '
            f'fill="none" stroke="{FG}" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round">{body}</svg>')

def txt(t, x, y, size=7, color=FG, anchor="middle"):
    return (f'<text x="{x}" y="{y}" font-family="DejaVu Sans" font-weight="bold" font-size="{size}" '
            f'fill="{color}" stroke="none" text-anchor="{anchor}">{t}</text>')

PLAY = f'<path d="M8 5.5v13l10.5-6.5z" fill="{ACC}" stroke="{ACC}"/>'
STOP = f'<rect x="6.5" y="6.5" width="11" height="11" rx="1.5" fill="{ACC}" stroke="{ACC}"/>'

def seek(n, back):
    # double chevron with a small label underneath
    d = "M12 6l-5 5 5 5M18 6l-5 5 5 5" if back else "M6 6l5 5-5 5M12 6l5 5-5 5"
    return f'<path d="{d}" stroke-width="1.7" transform="translate(0,-2)"/>' + txt(n, 12, 22.5, 6.5)

icons = {
    "player_play": svg(PLAY),
    "player_stop": svg(STOP),
    "back": svg('<path d="M14.5 6l-6 6 6 6"/>'),
    "forward": svg('<path d="M9.5 6l6 6-6 6"/>'),
    "player_rew": svg('<path d="M12 6l-6 6 6 6M18.5 6l-6 6 6 6"/>'),
    "player_fwd": svg('<path d="M5.5 6l6 6-6 6M12 6l6 6-6 6"/>'),
    "player_start": svg('<path d="M6.5 5.5v13M17.5 6l-8 6 8 6z"/>'),
    "player_end": svg('<path d="M17.5 5.5v13M6.5 6l8 6-8 6z"/>'),
    "prev_cut": svg('<path d="M12 4v16" stroke-dasharray="2 2.5"/><path d="M9 8l-4 4 4 4"/><path d="M15 8l4 4-4 4" opacity=".35"/>'),
    "next_cut": svg('<path d="M12 4v16" stroke-dasharray="2 2.5"/><path d="M15 8l4 4-4 4"/><path d="M9 8l-4 4 4 4" opacity=".35"/>'),
    "prev_black": svg(f'<rect x="10" y="6.5" width="10" height="11" rx="1.5" fill="#000" stroke="{FG}"/><path d="M7 8.5l-3.5 3.5 3.5 3.5"/>'),
    "next_black": svg(f'<rect x="4" y="6.5" width="10" height="11" rx="1.5" fill="#000" stroke="{FG}"/><path d="M17 8.5l3.5 3.5-3.5 3.5"/>'),
    "delete_selection": svg(f'<path d="M7 7l10 10M17 7L7 17" stroke="{RED}" stroke-width="2.1"/>'),
    "markA": svg(f'<path d="M8 5H5v14h3" stroke="{ACC}" stroke-width="2.1"/>' + txt("A", 16.5, 16.5, 11, ACC), 27, 24),
    "markB": svg(f'<path d="M19 5h3v14h-3" stroke="{ACC}" stroke-width="2.1"/>' + txt("B", 10.5, 16.5, 11, ACC), 27, 24),
    "goMarkA": svg(txt("A", 12, 16.5, 11, ACC) + f'<path d="M4 20h16" stroke="{ACC}" stroke-width="1.6"/>'),
    "goMarkB": svg(txt("B", 12, 16.5, 11, ACC) + f'<path d="M4 20h16" stroke="{ACC}" stroke-width="1.6"/>'),
    "backward1mn": svg(seek("1m", True)),
    "forward1mn": svg(seek("1m", False)),
    "backward1s": svg(seek("1s", True)),
    "forward1s": svg(seek("1s", False)),
    "backward2s": svg(seek("2s", True)),
    "forward2s": svg(seek("2s", False)),
    "backward4s": svg(seek("4s", True)),
    "forward4s": svg(seek("4s", False)),
    "fileopen": svg('<path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>'),
    "filesave": svg('<path d="M5 3h11l3 3v13a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z"/><path d="M7 3v5h8V3M7 21v-7h10v7"/>'),
    "info": svg('<circle cx="12" cy="12" r="9"/><path d="M12 11v5.5M12 7.8h.01" stroke-width="2.2"/>'),
    "play_filtered": svg(f'<rect x="3" y="5" width="18" height="14" rx="2.5"/><path d="M10 9v6l5-3z" fill="{FG}"/>'),
    "play_filtered_off": svg(f'<rect x="3" y="5" width="18" height="14" rx="2.5"/><path d="M7 9.5h10M9 12h6M11 14.5h2"/>'),
    "play_filtered_on": svg(f'<rect x="3" y="5" width="18" height="14" rx="2.5" stroke="{ACC}"/><path d="M7 9.5h10M9 12h6M11 14.5h2" stroke="{ACC}"/>'),
}
small = {
    "volume": svg(f'<path d="M3 6.5v3h2.5L9 12.5v-9L5.5 6.5z" fill="{FG}" stroke-width="1.2"/><path d="M11.5 5.5a3.5 3.5 0 0 1 0 5" stroke-width="1.4"/>', 16, 16),
    "volume_off": svg(f'<path d="M3 6.5v3h2.5L9 12.5v-9L5.5 6.5z" fill="{FG}" stroke-width="1.2"/><path d="M11 6l3.5 4M14.5 6L11 10" stroke="{RED}" stroke-width="1.4"/>', 16, 16),
}
icons.update(small)
# WsQ-only icons: side rail and widget sub-controls (new files, always 1x + 2x)
wsq = {
    "wsq_expand": svg('<path d="M6 6l6 6-6 6M13 6l6 6-6 6"/>'),
    "wsq_collapse": svg('<path d="M11 6l-6 6 6 6M18 6l-6 6 6 6"/>'),
    "wsq_video": svg('<rect x="3" y="6" width="13" height="12" rx="2"/><path d="M16 10l5-3v10l-5-3"/>'),
    "wsq_vfilter": svg('<path d="M4 6h16M7 12h10M10 18h4"/>'),
    "wsq_audio": svg('<path d="M4 10v4h4l5 4V6L8 10z"/><path d="M16 9a4 4 0 0 1 0 6"/>'),
    "wsq_afilter": svg('<path d="M4 6h16M7 12h10M10 18h4"/><path d="M19 15v5" stroke="#4a9bff"/>'),
    "wsq_format": svg('<path d="M14 3H6a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"/><path d="M14 3v6h6"/>'),
    "wsq_append": svg('<rect x="3" y="5" width="8" height="14" rx="1.5"/><path d="M15 12h6M18 9v6"/>'),
    "wsq_queue": svg('<path d="M4 6h16M4 12h16M4 18h10"/>'),
    "wsq_more": svg(f'<circle cx="5" cy="12" r="1.6" fill="{FG}" stroke="none"/><circle cx="12" cy="12" r="1.6" fill="{FG}" stroke="none"/><circle cx="19" cy="12" r="1.6" fill="{FG}" stroke="none"/>'),
    "wsq_play_dark": svg('<path d="M8 5.5v13l10.5-6.5z" fill="#0e1420" stroke="#0e1420"/>'),
    "wsq_stop_dark": svg('<rect x="6.5" y="6.5" width="11" height="11" rx="1.5" fill="#0e1420" stroke="#0e1420"/>'),
    "wsq_search": svg('<circle cx="11" cy="11" r="7"/><path d="M20 20l-4-4"/>'),
    "wsq_plus_dark": svg('<path d="M12 5v14M5 12h14" stroke="#0e1420" stroke-width="2.2"/>'),
    "wsq_save_dark": svg('<path d="M12 4v11M7 10l5 5 5-5M5 20h14" stroke="#0e1420" stroke-width="2.1"/>'),
    "wsq_save": svg('<path d="M12 4v11M7 10l5 5 5-5M5 20h14" stroke="#4a9bff" stroke-width="2.1"/>'),
    "wsq_arrow_down": svg('<path d="M1.5 1.5L5 5l3.5-3.5" stroke-width="1.6"/>', 10, 7),
    "wsq_arrow_up": svg('<path d="M1.5 5.5L5 2l3.5 3.5" stroke-width="1.6"/>', 10, 7),
    "wsq_check": svg('<path d="M2.5 6.5l2.5 2.5 5-6" stroke="#0e1420" stroke-width="2"/>', 12, 12),
}
for name, s in wsq.items():
    import re
    w = int(re.search(r'width="(\d+)"', s).group(1)); h = int(re.search(r'height="(\d+)"', s).group(1))
    for scale, suffix in ((1, ""), (2, "@2x")):
        cairosvg.svg2png(bytestring=s.encode(), write_to=os.path.join(OUT, f"{name}{suffix}.png"),
                         output_width=w * scale, output_height=h * scale)

for name, s in icons.items():
    import re
    w = int(re.search(r'width="(\d+)"', s).group(1)); h = int(re.search(r'height="(\d+)"', s).group(1))
    for scale, suffix in ((1, ""), (2, "@2x")):
        path = os.path.join(OUT, f"{name}{suffix}.png")
        if suffix and not os.path.exists(path):
            continue  # only refresh @2x variants that already exist in the resource list
        cairosvg.svg2png(bytestring=s.encode(), write_to=path, output_width=w * scale, output_height=h * scale)
print("generated", len(icons))
