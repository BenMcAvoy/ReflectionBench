# Render benchmark JSON as a well-formatted table.
import json
import sys
from typing import Any, Dict, List

# Preferred renderer: rich
HAVE_RICH = False
try:
    from rich.console import Console
    from rich.table import Table
    from rich.text import Text
    from rich.panel import Panel
    from rich.rule import Rule
    HAVE_RICH = True
except Exception:
    pass

# Fallback: tabulate
HAVE_TABULATE = False
if not HAVE_RICH:
    try:
        from tabulate import tabulate
        HAVE_TABULATE = True
    except Exception:
        pass


def compute_row(op: str, r_ns: float, e_ns: float) -> Dict[str, Any]:
    rttr_win = r_ns < e_ns
    speed = (e_ns / r_ns) if rttr_win else (r_ns / e_ns)
    return {
        "operation": op,
        "rttr": r_ns,
        "entt": e_ns,
        "winner": "RTTR" if rttr_win else "EnTT",
        "speed": speed,
        "rttr_win": rttr_win,
    }


def make_meter_text(rttr_win: bool, speed: float, width: int = 24) -> Text:
    # Centered bar: left=RTTR faster, right=EnTT faster; max ~3x shown
    if width % 2 != 0:
        width -= 1
    half = width // 2
    s = max(1.0, min(3.0, speed))
    filled = int(((s - 1.0) / 2.0) * half + 0.5)  # 1x->0, 3x->half
    left_fill = filled if rttr_win else 0
    right_fill = filled if not rttr_win else 0
    t = Text()
    # Left half
    if left_fill:
        t.append("█" * left_fill, style="cyan")
    t.append("░" * (half - left_fill), style="grey39")
    # Center marker
    t.append("|")
    # Right half
    t.append("░" * (half - right_fill), style="grey39")
    if right_fill:
        t.append("█" * right_fill, style="magenta")
    return t


def render_group_rich(console: Console, g: Dict[str, Any]) -> None:
    table = Table(title=g.get("name", ""), title_style="bold yellow", show_lines=False, pad_edge=False)
    table.add_column("operation", justify="left", no_wrap=True)
    table.add_column("RTTR ns/op", justify="right")
    table.add_column("EnTT ns/op", justify="right")
    table.add_column("winner", justify="left")
    table.add_column("speed", justify="right")
    table.add_column("advantage (left=RTTR | right=EnTT)", justify="left")

    rttrWins = enttWins = 0
    for r in g["rows"]:
        r_ns = float(r["rttr"]["ns_per_op"])
        e_ns = float(r["entt"]["ns_per_op"])
        row = compute_row(r["operation"], r_ns, e_ns)
        if row["rttr_win"]:
            rttrWins += 1
        else:
            enttWins += 1
        winner_style = "cyan" if row["rttr_win"] else "magenta"
        winner_txt = Text(row["winner"], style=winner_style)
        meter = make_meter_text(row["rttr_win"], row["speed"]) if HAVE_RICH else Text("")
        table.add_row(
            row["operation"],
            f"{row['rttr']:.2f}",
            f"{row['entt']:.2f}",
            winner_txt,
            f"x{row['speed']:.2f}",
            meter,
        )

    console.print(table)
    console.print(f"summary: [cyan]RTTR wins {rttrWins}[/cyan], [magenta]EnTT wins {enttWins}[/magenta]")
    console.print(Rule())


def render_group_tabulate(g: Dict[str, Any]) -> None:
    rows: List[List[str]] = []
    rttrWins = enttWins = 0
    for r in g["rows"]:
        r_ns = float(r["rttr"]["ns_per_op"])
        e_ns = float(r["entt"]["ns_per_op"])
        row = compute_row(r["operation"], r_ns, e_ns)
        if row["rttr_win"]:
            rttrWins += 1
        else:
            enttWins += 1
        # Simple ASCII meter
        width = 24
        half = width // 2
        s = max(1.0, min(3.0, row["speed"]))
        filled = int(((s - 1.0) / 2.0) * half + 0.5)
        left = ("#" * filled + "-" * (half - filled)) if row["rttr_win"] else ("-" * half)
        right = ("-" * (half - filled) + "#" * filled) if not row["rttr_win"] else ("-" * half)
        meter = f"{left}|{right}"
        rows.append([
            row["operation"],
            f"{row['rttr']:.2f}",
            f"{row['entt']:.2f}",
            row["winner"],
            f"x{row['speed']:.2f}",
            meter,
        ])
    print()
    print(g.get("name", ""))
    print(tabulate(rows, headers=["operation", "RTTR ns/op", "EnTT ns/op", "winner", "speed", "advantage (left=RTTR | right=EnTT)"], tablefmt="github", colalign=("left","right","right","left","right","left")))
    print(f"summary: RTTR wins {rttrWins}, EnTT wins {enttWins}")


def render_plain(g: Dict[str, Any]) -> None:
    # Basic padded fallback
    print()
    print(g.get("name", ""))
    print("-" * 88)
    header = f"{'operation':<20}{'RTTR ns/op':>14}{'EnTT ns/op':>14}  {'winner':<6}  {'speed':>6}  {'advantage':<24}"
    print(header)
    print("-" * 88)
    rttrWins = enttWins = 0
    for r in g["rows"]:
        r_ns = float(r["rttr"]["ns_per_op"]) 
        e_ns = float(r["entt"]["ns_per_op"]) 
        row = compute_row(r["operation"], r_ns, e_ns)
        if row["rttr_win"]:
            rttrWins += 1
        else:
            enttWins += 1
        width = 24
        half = width // 2
        s = max(1.0, min(3.0, row["speed"]))
        filled = int(((s - 1.0) / 2.0) * half + 0.5)
        left = ("#" * filled + "-" * (half - filled)) if row["rttr_win"] else ("-" * half)
        right = ("-" * (half - filled) + "#" * filled) if not row["rttr_win"] else ("-" * half)
        meter = f"{left}|{right}"
        print(f"{row['operation']:<20}{row['rttr']:>14.2f}{row['entt']:>14.2f}  {row['winner']:<6}  x{row['speed']:>5.2f}  {meter}")
    print("-" * 88)
    print(f"summary: RTTR wins {rttrWins}, EnTT wins {enttWins}")


def main() -> int:
    data = json.load(sys.stdin)
    if HAVE_RICH:
        console = Console()
        meta = data.get("meta", {})
        note = meta.get("note", "")
        console.print(Panel.fit("Meta reflection benchmark (rendered from JSON)\n" + note, title="Benchmark", title_align="left", border_style="yellow"))
        console.print("Legend: [bold]HOT[/bold] handles cached; [bold]COLD[/bold] per-iter lookup by name vs runtime hash → id.  speed: winner's speedup (x).  advantage: bar shows direction/magnitude (max ~3x).\n")
        for g in data.get("groups", []):
            render_group_rich(console, g)
    elif HAVE_TABULATE:
        print("Meta reflection benchmark (rendered from JSON)")
        for g in data.get("groups", []):
            render_group_tabulate(g)
    else:
        print("Meta reflection benchmark (rendered from JSON)")
        for g in data.get("groups", []):
            render_plain(g)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
