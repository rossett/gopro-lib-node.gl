import array
import math
import pynodegl as ngl
from pynodegl_utils.misc import scene


# TODO
# - more colors
# - full text occupation
# - padding between easings
# - share programs
# - instancing


def _block(w, h, program, **uniforms):
    block_width = (w, 0, 0)
    block_height = (0, h, 0)
    block_corner = (-w / 2., -h / 2., 0)
    block_quad = ngl.Quad( corner=block_corner, width=block_width, height=block_height)
    block_render = ngl.Render(block_quad, program)
    block_render.update_uniforms(**uniforms)
    return block_render

_colors = [
    (1, 0, 0, 1),
    (0, 1, 0, 1),
    (1, 0, 1, 1),
    (1, 1, 0, 1),
]


def _get_easing_node(cfg, easing, curve_zoom, palette_id, nb_points=64):
    main_color = _colors[palette_id]

    frag_data = cfg.get_frag('color')
    color_program = ngl.Program(fragment=frag_data, label='color')

    text_vratio = 1 / 6.
    anim_vratio = 1 / 8.

    area_size = 2
    width, height = area_size, area_size
    text_height = text_vratio * height
    anim_height = anim_vratio * height
    anim_geom_size = anim_height
    header_height = text_height + anim_height

    graph_size = area_size - header_height
    anim_width = graph_size
    normed_graph_size = graph_size * curve_zoom

    easing_block = _block(area_size, area_size, color_program,
                          color=ngl.UniformVec4(value=(.2, .2, .2, 1)))

    graph_block_render = _block(graph_size, graph_size, color_program,
                                color=ngl.UniformVec4(value=(.3, .3, .3, 1)))
    graph_block = ngl.Translate(graph_block_render, vector=(0, -header_height/2., 0))

    normed_graph_block_render = _block(normed_graph_size, normed_graph_size, color_program,
                                       color=ngl.UniformVec4(value=(.3, .2, .2, 1)))
    normed_graph_block = ngl.Translate(normed_graph_block_render, vector=(0, -header_height/2., 0))

    # Curve
    vertices_data = array.array('f')
    for i in range(nb_points + 1):
        t = i / float(nb_points)
        v = ngl.easing_evaluate(easing, t)
        x = t * width - width / 2.
        y = v * height - height / 2.
        vertices_data.extend([x, y, 0])
    vertices = ngl.BufferVec3(data=vertices_data)
    geometry = ngl.Geometry(vertices, topology='line_strip')
    render_curve = ngl.Render(geometry, color_program, label='curve')
    render_curve.update_uniforms(color=ngl.UniformVec4(main_color))

    curve_scale_factor = graph_size / area_size * curve_zoom
    curve = ngl.Scale(render_curve, factors=(curve_scale_factor, curve_scale_factor, 0))
    curve = ngl.Translate(curve, vector=(0, -header_height / 2., 0))

    # Legend
    legend_text = ngl.Text(text=easing,
                           fg_color=main_color,
                           bg_color=(0, 0, 0, 1),
                           box_corner=(-width / 2., height / 2. - text_height, 0),
                           box_width=(width, 0, 0),
                           box_height=(0, text_height, 0))
    legend_block_render = _block(width, text_height, color_program,
                                 color=ngl.UniformVec4(value=(0, 0, 0, 1)))
    legend_block = ngl.Translate(legend_block_render, vector=(0, (width - text_height) / 2, 0))
    legend = ngl.Group(children=(legend_block, legend_text))

    # Animation
    translate_animkf = [
        ngl.AnimKeyFrameVec3(0, (0, 0, 0)),
        ngl.AnimKeyFrameVec3(cfg.duration, (anim_width - anim_geom_size, 0, 0), easing),
    ]
    anim_geometry = ngl.Quad(
        corner=(-anim_width/2., 1 - header_height, 0),
        width=(anim_geom_size, 0, 0),
        height=(0, anim_geom_size, 0),
    )
    render_anim = ngl.Render(anim_geometry, color_program, label='anim %s' % easing)
    render_anim.update_uniforms(color=ngl.UniformVec4(main_color))
    anim = ngl.Translate(render_anim, anim=ngl.AnimatedVec3(translate_animkf))

    easing_group = ngl.Group(label=easing)
    easing_group.add_children(easing_block, legend, anim, graph_block, normed_graph_block, curve)

    return easing_group


_easing_specs = (
    ('linear',    0, .95),
    ('quadratic', 3, .95),
    ('cubic',     3, .95),
    ('quartic',   3, .95),
    ('quintic',   3, .95),
    ('sinus',     3, .95),
    ('exp',       3, .95),
    ('circular',  3, .95),
    ('bounce',    1, .95),
    ('elastic',   1, 0.5),
    ('back',      3, 0.8),
)


def _get_easing_list():
    easings = []
    for col, (base_name, flags, zoom) in enumerate(_easing_specs):
        versions = []
        if flags & 1:
            versions += ['_in', '_out']
        if flags & 2:
            versions += ['_in_out', '_out_in']
        if not flags:
            versions = ['']

        for version in versions:
            easings.append((base_name + version, zoom))
    return easings


def _get_easing_nodes(cfg):
    easings = _get_easing_list()

    nb_easings = len(easings)
    nb_rows = int(math.sqrt(nb_easings))
    nb_cols = int(math.ceil(nb_easings / float(nb_rows)))

    easing_h = 1. / nb_rows
    easing_w = 1. / nb_cols

    cfg.aspect_ratio = (nb_cols, nb_rows)

    for row in range(nb_rows):
        for col in range(nb_cols):
            easing_id = row * nb_cols + col
            if easing_id >= len(easings):
                return
            easing, zoom = easings[easing_id]

            easing_node = _get_easing_node(cfg, easing, zoom, easing_id % 4)
            easing_node = ngl.Scale(easing_node, factors=[easing_w, easing_h, 0])

            x = col * easing_w * 2
            y = row * easing_h * 2
            x_orig = -easing_w * nb_cols + easing_w
            y_orig =  easing_h * nb_rows - easing_h
            easing_node = ngl.Translate(easing_node, vector=(
                x_orig + x,
                y_orig - y,
                0))

            yield easing_node


@scene()
def all(cfg):
    cfg.duration = 2.

    group = ngl.Group()

    frag_data = cfg.get_frag('color')
    color_program = ngl.Program(fragment=frag_data, label='color')

    full_block = _block(2, 2, color_program,
                        color=ngl.UniformVec4(value=(.2, .2, .2, 1)))
    group.add_children(full_block)
    for easing_node in _get_easing_nodes(cfg):
        group.add_children(easing_node)

    #return ngl.GraphicConfig(group, blend=True,
    #                         blend_src_factor='src_alpha',
    #                         blend_dst_factor='one_minus_src_alpha',
    #                         blend_src_factor_a='zero',
    #                         blend_dst_factor_a='one')

    return group
