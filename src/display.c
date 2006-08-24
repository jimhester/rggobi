#include "RSGGobi.h"
#include "RUtils.h"

#include <gtk/gtk.h>

#include "vars.h"

USER_OBJECT_
RS_GGOBI(newScatterplot)(USER_OBJECT_ variables, USER_OBJECT_ datasetId)
{
 ggobid *gg;
 GGobiData *d;
 USER_OBJECT_ ans;
 displayd *display;

  d = toData(datasetId);
  g_return_val_if_fail(GGOBI_IS_DATA(d), NULL_USER_OBJECT);
  gg = d->gg;

 display = GGOBI(newScatterplot)(INTEGER_DATA(variables)[0],
                                 INTEGER_DATA(variables)[1],
                                 d, gg);
 display_add(display, gg);  
 ans = RS_displayInstance(display);

 return (ans);
}


USER_OBJECT_
RS_GGOBI(newParcoords)(USER_OBJECT_ variables, USER_OBJECT_ datasetId)
{
 ggobid *gg;
 GGobiData *d; 
 displayd *display;
 USER_OBJECT_ ans;
 gint *ids, n, i;

  d = toData(datasetId);
  g_return_val_if_fail(GGOBI_IS_DATA(d), NULL_USER_OBJECT);
  gg = d->gg;
 
 n  = GET_LENGTH(variables);
/*XXX make certain to free this. */
 ids = g_malloc0(n * sizeof (gint));
 for(i = 0; i < n; i++) 
   ids[i] = INTEGER_DATA(variables)[i];

 display = GGOBI(newParCoords)(ids, n, d, gg);
 display_add(display, gg);  
 ans = RS_displayInstance(display);

 return (ans);
}


/*
  Assumes the variables are specified in

 We want to add a general version that takes
 pairs of variables and generates the corresponding 
 plot for each of these pairs and then adds them to the
 scatter matrix.

 */
USER_OBJECT_
RS_GGOBI(newScatmat)(USER_OBJECT_ x, USER_OBJECT_ y, USER_OBJECT_ datasetId)
{
  ggobid *gg;
  GGobiData *d;
  displayd *display;
  USER_OBJECT_ ans;
  gint *rowIds, *colIds, nr, nc, i;

  d = toData(datasetId);
  g_return_val_if_fail(GGOBI_IS_DATA(d), NULL_USER_OBJECT);
  gg = d->gg;

  nr = GET_LENGTH(x);
  nc = GET_LENGTH(y);
 
  rowIds = g_malloc0 (nr * sizeof (gint));
  colIds = g_malloc0 (nc * sizeof (gint));
  for (i = 0; i < nr; i++) 
    rowIds[i] = INTEGER_DATA(x)[i];

  for (i = 0; i < nc; i++) 
    colIds[i] = INTEGER_DATA(y)[i];

  display = GGOBI(newScatmat)(rowIds, colIds, nr, nc, d, gg);
  display_add(display, gg);  
  ans = RS_displayInstance(display);

  return(ans);
}
USER_OBJECT_
RS_GGOBI(createPlot)(USER_OBJECT_ stype, USER_OBJECT_ svars, USER_OBJECT_ datasetId)
{
  GGobiData *d;
  ggobid *gg;
  displayd *display = NULL;
  GType type;
  GGobiExtendedDisplayClass *klass;

  d = toData(datasetId);
  g_return_val_if_fail(GGOBI_IS_DATA(d), NULL_USER_OBJECT);
  gg = d->gg;

  type = (GType) NUMERIC_DATA(stype)[0];
  klass = GGOBI_EXTENDED_DISPLAY_CLASS(g_type_class_peek(type));

  if(!klass) {
     PROBLEM "Unrecognized display type"
     ERROR;
  }

  if(klass->createWithVars && GET_LENGTH(svars)) {
     gint nvars, *vars, i;
     nvars = GET_LENGTH(svars);
     vars = g_malloc(sizeof(gint)*nvars);
     for(i = 0; i < nvars; i++)
       vars[i] = INTEGER_DATA(svars)[i] - 1;
     display = klass->createWithVars(false, nvars, vars, d, gg);
  } else if(klass->create)
     display = klass->create(false, NULL, d, gg);

  if(!display) {
        PROBLEM "Couldn't create the display"
	ERROR;
  }

  display_add(display, gg);

  return(RS_displayInstance(display));
}

USER_OBJECT_
RS_GGOBI(createDisplay)(USER_OBJECT_ smissing, USER_OBJECT_ dataset)
{
  ggobid *gg;
  GGobiData *data = NULL;
  displayd *dpy;
  USER_OBJECT_ ans;


  data = toData(dataset);
  g_return_val_if_fail(GGOBI_IS_DATA(data), NULL_USER_OBJECT);
  gg = data->gg;

  dpy = g_object_new(GGOBI_TYPE_EMBEDDED_DISPLAY, NULL);
  display_set_values(dpy, data, gg);

  display_add(dpy, gg);

  ans = toRPointer(dpy, "GGobiDisplay");

  return(ans);
}

/**
 This is called when we have reset all the variables in the different
 splots within a display.
 The intent is that this will recompute everything, including the 
 positions of the points/glyphs. Currently this is not doing that.
 Need to call some other method.
 */
USER_OBJECT_
RS_GGOBI(updateDisplay)(USER_OBJECT_ dpy, USER_OBJECT_ ggobiId)
{
  USER_OBJECT_ ans = NEW_LOGICAL(1);
  ggobid *gg;
  displayd *display;  

  gg = toGGobi(ggobiId);
  g_return_val_if_fail(GGOBI_IS_GGOBI(gg), NULL_USER_OBJECT);
  display = toDisplay(dpy);
  g_return_val_if_fail(GGOBI_IS_DISPLAY(display), NULL_USER_OBJECT);
  
  display_tailpipe(display, FULL,  gg);
  
  LOGICAL_DATA(ans)[0] = TRUE;
  return(ans);
}

/*
 Returns the number of splotd objects contained within a
 a given displayd object.
 */

USER_OBJECT_
RS_GGOBI(getNumPlotsInDisplay)(USER_OBJECT_ dpy)
{
  displayd *display;
  gint len;
  
  USER_OBJECT_ ans = NEW_INTEGER(1);
  display = toDisplay(dpy);
	g_return_val_if_fail(GGOBI_IS_DISPLAY(display), NULL_USER_OBJECT);
  len = g_list_length(display->splots);
  INTEGER_DATA(ans)[0] = len;

  return(ans);
}

USER_OBJECT_
RS_GGOBI(getDisplayOptions)(USER_OBJECT_ which)
{
  USER_OBJECT_ ans, names;
  gint NumOptions = 8;
  DisplayOptions *options;
  displayd *display;
  
  display = toDisplay(which);
  g_return_val_if_fail(GGOBI_IS_DISPLAY(display), NULL_USER_OBJECT);

  options = &(display->options);
  g_return_val_if_fail(options != NULL, NULL_USER_OBJECT);

  PROTECT(ans = NEW_LOGICAL(NumOptions));
  PROTECT(names = NEW_CHARACTER(NumOptions));

  LOGICAL_DATA(ans)[DOPT_POINTS] = options->points_show_p;
  SET_STRING_ELT(names, DOPT_POINTS, COPY_TO_USER_STRING("Show points"));
  LOGICAL_DATA(ans)[DOPT_AXES] = options->axes_show_p;
  SET_STRING_ELT(names, DOPT_AXES,  COPY_TO_USER_STRING("Show axes"));

  LOGICAL_DATA(ans)[DOPT_AXESLAB] = options->axes_label_p;
  SET_STRING_ELT(names, DOPT_AXESLAB,
    COPY_TO_USER_STRING("Show tour axes"));
  LOGICAL_DATA(ans)[DOPT_AXESVALS] = options->axes_values_p;
  SET_STRING_ELT(names, DOPT_AXESVALS,
    COPY_TO_USER_STRING("Show axes labels"));

  LOGICAL_DATA(ans)[DOPT_EDGES_U] = options->edges_undirected_show_p;
  SET_STRING_ELT(names, DOPT_EDGES_U, COPY_TO_USER_STRING("Undirected edges"));
  LOGICAL_DATA(ans)[DOPT_EDGES_A] = options->edges_arrowheads_show_p;
  SET_STRING_ELT(names, DOPT_EDGES_A, COPY_TO_USER_STRING("Arrowheads"));
  LOGICAL_DATA(ans)[DOPT_EDGES_D] = options->edges_directed_show_p;
  SET_STRING_ELT(names, DOPT_EDGES_D, COPY_TO_USER_STRING("Directed edges"));

  LOGICAL_DATA(ans)[DOPT_WHISKERS] = options->whiskers_show_p;
  SET_STRING_ELT(names, DOPT_WHISKERS,
    COPY_TO_USER_STRING("Show whiskers"));

/* unused
  LOGICAL_DATA(ans)[5] = options->missings_show_p;
  SET_STRING_ELT(names, 5, COPY_TO_USER_STRING("Missing Values"));
  LOGICAL_DATA(ans)[8] = options->axes_center_p;
  SET_STRING_ELT(names, 8,  COPY_TO_USER_STRING("Center axes"));
  LOGICAL_DATA(ans)[9] = options->double_buffer_p;
  SET_STRING_ELT(names, 9,  COPY_TO_USER_STRING("Double buffer"));
  LOGICAL_DATA(ans)[10] = options->link_p;
  SET_STRING_ELT(names, 10,  COPY_TO_USER_STRING("Link"));
*/

  SET_NAMES(ans, names);

  UNPROTECT(2);

  return(ans);
}
/*

 */
USER_OBJECT_
RS_GGOBI(setDisplayOptions)(USER_OBJECT_ which, USER_OBJECT_ values)
{
  gint i;
  DisplayOptions *options;
  displayd *display;
  int apply = 0;

  g_return_val_if_fail(GET_LENGTH(values) == 8, NULL_USER_OBJECT);
  
  if(GET_LENGTH(which) == 0) {
     options = GGOBI(getDefaultDisplayOptions)();
  } else {
     display = toDisplay(which);
     g_return_val_if_fail(GGOBI_IS_DISPLAY(display), NULL_USER_OBJECT);
     options = &(display->options);
     g_return_val_if_fail(options != NULL, NULL_USER_OBJECT);
     apply = 1;
  }
  
  i = 0;
  options->points_show_p = LOGICAL_DATA(values)[i++];
  options->axes_show_p = LOGICAL_DATA(values)[i++];
  options->axes_label_p = LOGICAL_DATA(values)[i++];
  options->axes_values_p = LOGICAL_DATA(values)[i++];
  options->edges_undirected_show_p = LOGICAL_DATA(values)[i++];
  options->edges_arrowheads_show_p = LOGICAL_DATA(values)[i++];
  options->edges_directed_show_p = LOGICAL_DATA(values)[i++];
  options->whiskers_show_p = LOGICAL_DATA(values)[i++];
/* unused
  options->missings_show_p = LOGICAL_DATA(values)[i++];
  options->axes_center_p = LOGICAL_DATA(values)[i++];
  options->double_buffer_p = LOGICAL_DATA(values)[i++];
  options->link_p = LOGICAL_DATA(values)[i++];
*/

  if(apply) {
    set_display_options(display, display->ggobi);
  }

  return (NULL_USER_OBJECT);
}

USER_OBJECT_
RS_GGOBI(getDisplayDataset)(USER_OBJECT_ dpy)
{
 displayd * display;
 USER_OBJECT_ ans;

  display = toDisplay(dpy);
	g_return_val_if_fail(GGOBI_IS_DISPLAY(display), NULL_USER_OBJECT);
  ans = RS_datasetInstance(display->d);
  return(ans);
}

USER_OBJECT_
RS_displayInstance(displayd *display)
{
  USER_OBJECT_ ans;
  ans = toRPointer(display, "GGobiDisplay");
  return(ans);
}

USER_OBJECT_ 
RS_GGOBI(closeDisplay)(USER_OBJECT_ ref, USER_OBJECT_ ggobiId)
{
  ggobid *gg = toGGobi(ggobiId);
  g_return_val_if_fail(GGOBI_IS_GGOBI(gg), NULL_USER_OBJECT);
  USER_OBJECT_ ans = NEW_LOGICAL(1);
  displayd *display;

  if(!gg)
    return(ans);

  display  = (displayd *) R_ExternalPtrAddr(ref); 
  display = ValidateDisplayRef(display, gg, false);
  if(display) {
    display_free(display, true, gg);
    LOGICAL_DATA(ans)[0] = TRUE;
    gdk_flush();
  } 

  return(ans);
}

displayd *
toDisplay(USER_OBJECT_ rdisplay)
{
  if(inherits(rdisplay, "ggobiDisplay")) {
    displayd *display = getPtrValue(rdisplay);
    g_return_val_if_fail(GGOBI_IS_DISPLAY(display), NULL);
    g_return_val_if_fail(ValidateGGobiRef(display->ggobi, false) != NULL, NULL);
    return(ValidateDisplayRef(display, display->ggobi, false));
  }
  g_critical("An R GGobi display object must inherit from class 'ggobiDisplay'");
  return(NULL);
}

/**
 Probably should be deprecated.
 */
void
RS_GGOBI(raiseDisplay)(glong *plotId, glong *numEls, glong *raiseOrIcon,
  glong *up, glong *ggobiId)
{
  ggobid *gg = ggobi_get(*ggobiId);
  gint i;

  for (i = 0; i < *numEls; i++) {
    plotId[i] = GGOBI(raiseWindow)(plotId[i], raiseOrIcon[0], up[0], gg);
  }
}
