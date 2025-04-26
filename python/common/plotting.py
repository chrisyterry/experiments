import plotly.graph_objects as go

# make the axes of a plotly figure equal
def makeAxesEqual(fig:go.Figure):
  axis_lims = [float('inf'), -float('inf')]
  # for each plot on the axes
  for plot in fig.data:
     axis_lims[0] = min(axis_lims[0], min(plot.x))
     axis_lims[0] = min(axis_lims[0], min(plot.y))
     axis_lims[0] = min(axis_lims[0], min(plot.z))
     axis_lims[1] = max(axis_lims[1], max(plot.x))
     axis_lims[1] = max(axis_lims[1], max(plot.y))
     axis_lims[1] = max(axis_lims[1], max(plot.z))

  fig.update_layout(scene=dict(
                    aspectmode='cube',
                    xaxis=dict(range=[axis_lims[0]*1.05, axis_lims[1]*1.05]),
                    yaxis=dict(range=[axis_lims[0]*1.05, axis_lims[1]*1.05]),
                    zaxis=dict(range=[axis_lims[0]*1.05, axis_lims[1]*1.05])))