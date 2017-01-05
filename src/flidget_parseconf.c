#include "flidget.h"
#include <ctype.h>

int skip_to_next(FILE *f,char needle) {
  int cc;
  do {
    cc=fgetc(f);
  } while (cc!=needle && cc!=EOF);
  if (cc==needle) {
    ungetc(cc,f);
    return 1;
  } else {
    return 0;
  }
}

int skip_spaces(FILE *f) {
  int cc;
  do {
    cc=fgetc(f);
  } while (isspace(cc) && cc!=EOF);
  if (!isspace(cc)) {
    ungetc(cc,f);
    return 1;
  } else {
    return 0;
  }
}

int get_label(FILE *f,char **label) {
  int res;
  if (!skip_to_next(f,'[')) {
    return 0;
  }
  //ignore [
  fgetc(f);
  //read
  res=fscanf(f,"%ms\n",label);
  if (res==EOF) {
    return 0;
  }
  if (res!=1) {
    perror("fscanf");
    return 0;
  };
  //remove ]
  if ((*label)[strlen(*label)-1]!=']') {
    printf("syntax error\n");
    return 0;
  }
  (*label)[strlen(*label)-1]=0;
  //printf("get_label: %s\n",*label);
  return 1;
}

enum conf_status get_param(FILE *f, char **name, char **value) {
  char c;
  int res;
  if (!skip_spaces(f)) {
    return END;
  }
  c=fgetc(f);
  if (c=='[') {
    ungetc(c,f);
    return NEWWIDGET;
  }
  ungetc(c,f);
  //read
  res=fscanf(f,"%m[^=]=%m[^\n\r]",name,value);
  if (res==EOF) {
    return END;
  }
  if (res!=2) {
    perror("syntax error:");
    return ERROR;
  };
  //printf("get_param: %s = %s\n",*name,*value);
  return INWIDGET;
}

enum conf_status parse_widget(struct flidget_ws *ws) {
  struct chained_ws *curr_ws;
  char *label=NULL;
  char **names;
  char **values;
  char *name=NULL;
  char *value=NULL;
  int nb_params,nb_alloc_params;
  enum conf_status res=INWIDGET;
  int i;
  // got to next label, otherwise end
  if (!get_label(ws->conf,&label)) {
    return END;
  }
  //printf("\nparse_widget: label = %s\n",label);
  // parse params
  nb_params=0;
  nb_alloc_params=1;
  names=malloc(sizeof(char*)*nb_alloc_params);
  values=malloc(sizeof(char*)*nb_alloc_params);
  while (get_param(ws->conf,&name,&value)==INWIDGET) {
    nb_params++;
    if (nb_params>nb_alloc_params) {
      nb_alloc_params+=2;
      names=realloc(names,sizeof(char*)*nb_alloc_params);
      values=realloc(values,sizeof(char*)*nb_alloc_params);
    }
    names[nb_params-1]=name;
    values[nb_params-1]=value;
  }
  for (i=0;i<nb_params;i++) {
    //printf("parse_widget: %s = %s\n",names[i],values[i]);
    if (!strcmp(names[i],"type") && (!strcmp(values[i],"console") || !strcmp(values[i],"graph"))) {
      // allocate and chain workspace
      curr_ws=malloc(sizeof(struct chained_ws));
      memset(curr_ws,0,sizeof(struct chained_ws));
      if (ws->wss==NULL) { // first widget
        curr_ws->next=NULL;
      } else {
        curr_ws->next=ws->wss;
      }
      curr_ws->type=values[i];
      ws->wss=curr_ws;
      if (!strcmp(values[i],"console")) {
        curr_ws->ws=malloc(sizeof(struct console_ws));
        ((struct console_ws*)(curr_ws->ws))->label=label;
        init_console_ws((struct console_ws*)curr_ws->ws,nb_params,names,values);
        break;
      }
      if (!strcmp(values[i],"graph")) {
        curr_ws->ws=malloc(sizeof(struct monitor_ws));
        ((struct monitor_ws*)(curr_ws->ws))->label=label;
        init_monitor_ws((struct monitor_ws*)curr_ws->ws,nb_params,names,values);
        break;
      }
    }
  }
  return res;
}

