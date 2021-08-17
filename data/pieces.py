"""hack/script to convert pieces.txt to array initializer syntax

pieces.txt is trivially derived from the source text of
https://tetris.fandom.com/wiki/Orientation

In that article, "o" drawn at the wrong y-position relative to the others
according to the spawn behavior described in
https://tetris.fandom.com/wiki/Tetris_Guideline . This emits "o" with an
adjusted y-offset.

"""

import sys
from collections import defaultdict
from itertools import cycle
from functools import partial

pieces = defaultdict(lambda: defaultdict(dict))

index_gen = cycle(reversed(range(4)))
orientation_gen = cycle(range(4))
orientation = -1
for line in sys.stdin:
    if '[' not in line:
        continue
    row = line.strip("[]\n").split(",")
    row_index = next(index_gen)
    if (row_index == 3):
        orientation = next(orientation_gen)
    tet0 = set(c for c in row if c != " ")
    assert len(tet0) in {0, 1}, tet0
    if not tet0:
        continue
    tet = next(iter(tet0))
    pieces[tet][orientation][row_index] = [i for i, c in enumerate(row) if c == tet]

from pprint import pprint

tets = "zlosijt"
for tet in tets:
    orientations = pieces[tet]
    print("[(int)tetris::tet::" + str(tet) + "] = {")
    for orientation, row_columns in orientations.items():
        row_offset = 1 if tet == "o" else 0

        offsets = list(
            (column, row + row_offset)
            for row, columns in row_columns.items()
            for column in columns
        )
        assert len(offsets) == 4
        eprint = partial(print, end='')
        eprint("  {")
        for i, x_y in enumerate(offsets):
            eprint("{" + str(x_y[0]) + ", " + str(x_y[1]) + "}")
            if i < 3:
                eprint(", ")
        print("},")
    print("},")

#pprint(pieces)
