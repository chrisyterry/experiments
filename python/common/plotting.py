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
  
def plotObjects(objects:list):
   """
   Create a plot for the specified objects, if the objects have a getRenderingPoints method
   """
   # create the figure
   fig = go.Figure()
   # for each object (assumed to be a tuple with an object in the first slot and an optional settings vector in the second slot)
   for object in objects:
      object_plot_points = object[0].getRenderingPoints()
      print(object_plot_points)
      object_plot = go.Scatter3d(x=object_plot_points[:,0], y=object_plot_points[:,1], z=object_plot_points[:,2], mode='lines', line = object[1]) # line=dict(color=triangle_color)
      fig.add_trace(object_plot)

   makeAxesEqual(fig)
   return fig