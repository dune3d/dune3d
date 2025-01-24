import sys
sys.path.append('build')
import dune3d_py as dune3d
doc = dune3d.Document.new_from_file('/tmp/text.d3ddoc')
doc.render_texts()
doc.get_groups_sorted()[-1].solid_model.export_stl("/tmp/text.stl")
