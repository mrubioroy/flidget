#include "flidget.h"

int init_console_ws(struct console_ws *ws,int nb_params,char **names,char **values) {
  int i;
  for (i=0;i<nb_params;i++) {
    if (!strcmp(names[i],"command")) {
      ws->cmd=values[i];
    }
    if (!strcmp(names[i],"left")) {
      ws->posX=atoi(values[i]);
    }
    if (!strcmp(names[i],"top")) {
      ws->posY=atoi(values[i]);
    }
    if (!strcmp(names[i],"width")) {
      ws->width=atoi(values[i]);
      if (ws->width<=0) {
        printf("warning on %s: bad value for parameter width. must be > 0. ignoring widget...\n",ws->label);
        return 0;
      }
    }
    if (!strcmp(names[i],"height")) {
      ws->height=atoi(values[i]);
      if (ws->height<=0) {
        printf("warning on %s: bad value for parameter height. must be > 0. ignoring widget...\n",ws->label);
        return 0;
      }
    }
    if (!strcmp(names[i],"buffer")) {
      ws->data_size_max=atoi(values[i]);
      if (ws->height<=0) {
        printf("warning on %s: bad value for parameter buffer. must be > 0. ignoring widget...\n",ws->label);
        return 0;
      }
    }
  }
  return 1;
}

int init_monitor_ws(struct monitor_ws *ws,int nb_params,char **names,char **values) {
  int i;
  for (i=0;i<nb_params;i++) {
    if (!strcmp(names[i],"command")) {
      ws->cmd=values[i];
    }
    if (!strcmp(names[i],"left")) {
      ws->posX=atoi(values[i]);
    }
    if (!strcmp(names[i],"top")) {
      ws->posY=atoi(values[i]);
    }
    if (!strcmp(names[i],"width")) {
      ws->width=atoi(values[i]);
      if (ws->width<=0) {
        printf("warning on %s: bad value for parameter width. must be > 0. ignoring widget...\n",ws->label);
        return 0;
      }
    }
    if (!strcmp(names[i],"height")) {
      ws->height=atoi(values[i]);
      if (ws->height<=0) {
        printf("warning on %s: bad value for parameter height. must be > 0. ignoring widget...\n",ws->label);
        return 0;
      }
    }
    if (!strcmp(names[i],"max")) {
      ws->max=(unsigned int)atoi(values[i]);
    }
    if (!strcmp(names[i],"units")) {
      ws->units=values[i];
    }
    if (!strcmp(names[i],"nb_streams")) {
      ws->nb_streams=atoi(values[i]);
      if (ws->nb_streams<=0) {
        printf("warning on %s: bad value for parameter nb_streams. must be > 0. ignoring widget...\n",ws->label);
        return 0;
      }
    }
  }
  return 1;
}

int launch_ds(char* name,char *cmd) {
  pid_t pid;
  int pipes[2];

  if (pipe(pipes)==-1) {
    perror("pipes");
    exit(1);
  }
  pid=fork();
  if (pid==-1) {
    perror("fork");
    exit(1);
  } else if (pid==0) {
    printf("%s (%d) calling %s...\n",name,getpid(),cmd);
    while ((dup2(pipes[1],STDOUT_FILENO)==-1) && (errno==EINTR)) {}
    close(pipes[1]);
    close(pipes[0]);
    execl("/bin/sh","sh","-c",cmd,NULL);
    perror("execlp");
    _exit(1);
  }
  close(pipes[1]);
  return pipes[0];
}

static void activate(GtkApplication *app, void *workspace) {
  struct flidget_ws *ws=(struct flidget_ws*)workspace;
  struct chained_ws *iws;
  struct console_ws *c_ws;
  struct monitor_ws *m_ws;
  char *css_filename;
  GFile *css_file;
  GtkCssProvider *css_provider;

  //load global CSS provider
  css_filename=malloc(strlen(ws->config_folder)+15);
  sprintf(css_filename,"%s/flidget.css",ws->config_folder);
  css_file=g_file_new_for_path(css_filename);
  css_provider=gtk_css_provider_new();
  gtk_css_provider_load_from_file(css_provider,css_file,NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),GTK_STYLE_PROVIDER(css_provider),GTK_STYLE_PROVIDER_PRIORITY_THEME);

  free(css_filename);
  g_object_unref(css_file);

  // iterate chained ws
  iws=ws->wss;
  while (iws!=NULL) {
    if (!strcmp(iws->type,"console")) {
      c_ws=(struct console_ws*)iws->ws;
/*
      printf("widget %s of type %s at x:%d,y:%d w:%d,h:%d\n",c_ws->label,iws->type,c_ws->posX,c_ws->posY,c_ws->width,c_ws->height);
      printf("     command: %s\n",c_ws->cmd);
      printf("     buffer size: %d\n",c_ws->data_size_max);
*/
      activate_console(app,c_ws);
    }
    if (!strcmp(iws->type,"graph")) {
      m_ws=(struct monitor_ws*)iws->ws;
/*
      printf("widget %s of type %s at x:%d,y:%d w:%d,h:%d\n",m_ws->label,iws->type,m_ws->posX,m_ws->posY,m_ws->width,m_ws->height);
      printf("     command: %s\n",m_ws->cmd);
      printf("     max:%d, units:%s, nb_streams:%d\n",m_ws->max,m_ws->units,m_ws->nb_streams);
*/
//      m_ws->style=gtk_style_context_new();
//      gtk_style_context_add_provider(m_ws->style,GTK_STYLE_PROVIDER(css_provider),GTK_STYLE_PROVIDER_PRIORITY_THEME);
      activate_monitor(app,m_ws);
    }
    iws=iws->next;
  }
}

void sig_handler(int sig) {
  pid_t pid=wait(NULL);
  printf("pid %d ended\n",pid);
}

int main(int argc,char**argv) {
  GtkApplication *app;
  int status;
  char *config_file;
  char *config_folder;
  char *home_dir;
  struct flidget_ws *ws=malloc(sizeof(struct flidget_ws));
  memset(ws,0,sizeof(struct flidget_ws));

  setbuf(stdout, NULL);
  signal(SIGCHLD,sig_handler);

  if (argc>1 && !strcmp(argv[1],"--help")) {
    printf("usage: %s config_folder\n",argv[0]);
    return 0;
  }
  if (argc>1) {
    config_folder=argv[1];
  } else {
    config_folder=strdup("~/.config/flidget");
  }

  if (config_folder[0]=='~') {
    home_dir=getenv("HOME");
    ws->config_folder=malloc(strlen(home_dir)+strlen(config_folder)+2);
    sprintf(ws->config_folder,"%s/%s",home_dir,config_folder+2);
  } else {
    ws->config_folder=config_folder;
  }
  config_file=malloc(strlen(ws->config_folder)+20);
  sprintf(config_file,"%s/flidget.conf",ws->config_folder);

  // load config
  if ((ws->conf=fopen(config_file,"r"))==NULL) {
    printf("Error opening config file at %s\n",config_file);
    printf("Flidget includes an example of a configuration file\n");
    printf("See the man page for more details\n");
    perror("open");
    return 1;
  }
  free(config_file);

  // parse config and populate wss
  while (parse_widget(ws)!=END);

  // disable GTK treating command line arguments
  //argc=0;
  //argv=NULL;

  // new GTK app
  app=gtk_application_new(NULL,G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app,"activate",G_CALLBACK(activate),(void*)ws);
  status=g_application_run(G_APPLICATION(app),argc,argv);
  g_object_unref(app);
  return status;
}
