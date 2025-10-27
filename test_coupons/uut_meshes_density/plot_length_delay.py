#!/usr/bin/env python3

import math
import pathlib
from collections import defaultdict

import matplotlib.cm
import gerbonara
from gerbonara.utils import MM, Tag
import click


@click.command()
@click.option('-o', '--overlay', multiple=True, type=click.Path(path_type=pathlib.Path, exists=True, dir_okay=False))
@click.option('-a', '--anchor', multiple=True)
@click.option('-i', '--interval', type=float, default=5)
@click.option('--text/--no-text', default=True)
@click.argument('in_gerber', type=click.Path(path_type=pathlib.Path, exists=True, dir_okay=False))
@click.argument('out_svg', type=click.Path(path_type=pathlib.Path, dir_okay=False))
def cli(in_gerber, overlay, anchor, interval, out_svg, text):
    grb = gerbonara.rs274x.GerberFile.open(in_gerber)
    anchor = [y for x in anchor for y in x.split(',')]

    line_map = defaultdict(lambda: [])
    anchor_pos = []

    for obj in grb.objects:
        if isinstance(obj, gerbonara.graphic_objects.Flash):
            if (part_info := obj.attrs.get('.P')):
                component, number, *function = part_info
                for elem in anchor:
                    if f'{component}.{number}' == elem:
                        x, y = MM(obj.x, obj.unit), MM(obj.y, obj.unit)
                        anchor_pos.append((elem, (round(x * 100), round(y * 100))))
        elif isinstance(obj, gerbonara.graphic_objects.Line):
            x1, y1 = MM(obj.x1, obj.unit), MM(obj.y1, obj.unit)
            x2, y2 = MM(obj.x2, obj.unit), MM(obj.y2, obj.unit)

            key1 = (round(x1 * 100), round(y1 * 100))
            key2 = (round(x2 * 100), round(y2 * 100))

            line_map[key1].append((key2, obj))
            line_map[key2].append((key1, obj))

    tags = []
    lines = []
    for anchor_name, start_pos in anchor_pos:
        stack, visited = [(0, 0, start_pos)], [start_pos]
        current_polyline = None
        lines.append([])
        while stack:
            #print(stack)
            distance, last_tag, pos = stack.pop()
            visited.append(pos)
            x1, y1 = pos
            x1, y1 = x1/100, y1/100

            if len(line_map.get(pos, [])) < 2 and pos != start_pos:
                print(f'Anchor {anchor_name}: start at {start_pos}, end at {pos}, total distance {distance:.2f}mm')
            for far, obj in line_map.get(pos, []):
                if far not in visited:
                    x2, y2 = far
                    x2, y2 = x2/100, y2/100
                    length = math.dist((x1, y1), (x2, y2))
                    ang = math.degrees(math.atan2(y2-y1, x2-x1))
                    if abs(ang) > 90:
                        ang += 180
                    if last_tag + length > interval:
                        xform = {'transform': f'rotate({ang:.3f}) scale(1 -1)'} #'transform': 'scale(1, -1)', 'transform_origin': 'center'}
                        #if abs(x2 - x1) < abs(y2 - y1):
                        #    xform = {'transform': 'rotate(-90)', 'transform_origin': 'center'}

                        # produce tag in the middle
                        def do_text(**kw):
                            tags.append(Tag('g',
                                            [Tag('text', 
                                                [Tag('tspan',
                                                     [f'{distance + length/2:.2f}'],
                                                     dominant_baseline='middle')],
                                                x='0',
                                                y='0',
                                                text_anchor='middle',
                                                **xform,
                                                **kw,
                                                style=f'font: 0.2px bold sans-serif; fill: black')
                                            ],
                                            transform=f'translate({(x1+x2)/2:.3f} {(y1+y2)/2:.3f})'))

                        if text:
                            do_text(stroke='white', stroke_width='0.02mm')
                            do_text()

                        if current_polyline:
                            lines[-1].append(current_polyline.to_svg())
                        current_polyline = gerbonara.cam.Polyline(*obj.to_primitives())

                        stack.insert(0, (distance + length, length/2, far))
                    else:

                        if current_polyline is None:
                            current_polyline = gerbonara.cam.Polyline(*obj.to_primitives())
                        elif not current_polyline.append(*obj.to_primitives()):
                            if current_polyline:
                                lines[-1].append(current_polyline.to_svg())
                            current_polyline = gerbonara.cam.Polyline(*obj.to_primitives())

                        stack.insert(0, (distance + length, last_tag + length, far))

        if current_polyline:
            lines[-1].append(current_polyline.to_svg())

        for i, path in enumerate(lines[-1]):
            f = i/(len(lines[-1])-1)
            n = 10
            f = abs((((n*f)*2) % 2) - 1)
            r, g, b, _a = matplotlib.cm.viridis(f)
            path.attrs['stroke'] = f'#{round(r*255):02x}{round(g*255):02x}{round(b*255):02x}'

    grb.objects = [obj for obj in grb.objects if not isinstance(obj, gerbonara.graphic_objects.Line)]
    svg = grb.to_svg(margin=3)

    svg.children[0].children += sum(lines, start=[])
    svg.children[0].children += tags

    for fn in overlay:
        grb = gerbonara.rs274x.GerberFile.open(fn)
        for tag in grb.svg_objects(fg='red'):
            if tag.name == 'path' and float(tag.attrs.get('stroke_width', 1000)) < 0.2:
                tag.attrs['stroke_width'] = '0.2'
            svg.children.append(tag)

    out_svg.write_text(str(svg))


if __name__ == '__main__':
    cli()
