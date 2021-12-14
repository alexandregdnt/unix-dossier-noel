#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <mysql.h>
#include <setjmp.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ,idShm,idSem;
TAB_CONNEXIONS *tab;
struct sigaction  Action;

void afficheTab();
void handlerSIGINT(int sig);

MYSQL* connexion;

int main()
{
  // Connection à la BD
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(SERVEUR) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Armement des signaux
  Action.sa_handler = handlerSIGINT;
  sigemptyset(&Action.sa_mask);  
  Action.sa_flags = 0;
  sigaction(SIGINT, &Action, NULL);

  // creation des ressources
  fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }

  // Initilisation de la table de connexions
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i=0 ; i<6 ; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom,"");
    for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
    tab->connexions[i].pidModification = 0;
  }
  tab->pidServeur1 = getpid();
  tab->pidServeur2 = 0;
  tab->pidAdmin = 0;
  tab->pidPublicite = 0;

  afficheTab();

  // Creation du processus Publicite

  int i,k,j,n;
  MESSAGE m;
  MESSAGE reponse;
  char requete[200];
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  PUBLICITE pub;

  while(1)
  {
  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv - 1");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }

    switch(m.requete)
    {
      case CONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                      
                      i = 0;
                      
                      while(i < 6 && tab->connexions[i].pidFenetre != 0)
                      {
                        i++;
                      }
                      
                      if(tab->connexions[i].pidFenetre == 0)
                      {
                        tab->connexions[i].pidFenetre = m.expediteur;
                      }
                      
                      break; 

      case DECONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);

                      i = 0;
                      
                      while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                      {
                        i++;
                      }
                      
                      if(tab->connexions[i].pidFenetre == m.expediteur)
                      {
                        tab->connexions[i].pidFenetre = 0;
                        strcpy(tab->connexions[i].nom,"");
                        for (j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
                        tab->connexions[i].pidModification = 0;
                      }

                      break; 

      case LOGIN :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
                      
                      int pid_tmp;
                      pid_tmp = m.expediteur;
                      char nom_tmp[20];
                      strcpy(nom_tmp, m.data1);
                      
                      sprintf(requete,"select nom from UNIX_FINAL where nom='%s' and motdepasse='%s';",m.data1,m.data2);
                      mysql_query(connexion,requete);
                      resultat = mysql_store_result(connexion);

                      if(resultat)
                      {
                        tuple = mysql_fetch_row(resultat);
                        m.type = m.expediteur;
                        m.expediteur = 1;
                        m.requete = LOGIN;
                        if(tuple != NULL && strcmp(m.data1, tuple[0]) == 0)
                        {
                          i = 0;
                      
                          while(i < 6 && tab->connexions[i].pidFenetre != m.type)
                          {
                            i++;
                          }
                          
                          if(tab->connexions[i].pidFenetre == m.type)
                          {
                            strcpy(tab->connexions[i].nom, m.data1);
                          }

                          strcpy(m.data1, "OK");
                        }
                        else
                        {
                          strcpy(m.data1, "KO");
                        }

                        if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          msgctl(idQ, IPC_RMID, NULL);
                          perror("(SERVEUR) Erreur de msgsnd - 1");
                          exit(1);
                        }

                        kill(m.type, SIGUSR1);     
                      }


                      if (strcmp(m.data1,"OK") == 0)
                      {
                        i = 0;

                        while(i < 6)
                        {
                          if(strcmp(tab->connexions[i].nom, "") != 0 && strcmp(tab->connexions[i].nom, nom_tmp) != 0)
                          {
                          
                            m.type = tab->connexions[i].pidFenetre;
                            m.expediteur = 1;
                            m.requete = ADD_USER;
                            strcpy(m.data1, nom_tmp);
                            if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                            {
                              msgctl(idQ, IPC_RMID, NULL);
                              perror("(SERVEUR) Erreur de msgsnd - 2");
                              exit(1);
                            }
                            kill(m.type, SIGUSR1);

                            sleep(1);

                            m.type = pid_tmp;
                            strcpy(m.data1, tab->connexions[i].nom);

                            if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                            {
                              msgctl(idQ, IPC_RMID, NULL);
                              perror("(SERVEUR) Erreur de msgsnd - 3");
                              exit(1);
                            }
                            kill(m.type, SIGUSR1);

                          }
                          i++;
                        }  
                      }
              
                      break; 

      case LOGOUT :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);

                      i = 0;
                      char data1_tmp[20];
                      
                      while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                      {
                        i++;
                      }
                      
                      if(tab->connexions[i].pidFenetre == m.expediteur)
                      {
                        strcpy(data1_tmp, tab->connexions[i].nom);
                        strcpy(tab->connexions[i].nom,"");
                        for (j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
                        tab->connexions[i].pidModification = 0;
                      }

                        i = 0;

                        while(i < 6)
                        {
                          if(strcmp(tab->connexions[i].nom, "") != 0)
                          {
                            
                            //Enlever le pid de la table du serveur (linge des usitilisateurs accepte)
                            j = 0;
                            while(j < 5 && tab->connexions[i].autres[j] != m.expediteur)
                            {
                              j++;
                            }
                            if(tab->connexions[i].autres[j] == m.expediteur)
                            {
                              tab->connexions[i].autres[j] = 0;
                            }


                            //Enlever le pid des listes des clients
                            reponse.type = tab->connexions[i].pidFenetre;
                            reponse.expediteur = 1;
                            reponse.requete = REMOVE_USER;
                            strcpy(reponse.data1, data1_tmp);
                            if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                            {
                              msgctl(idQ, IPC_RMID, NULL);
                              perror("(SERVEUR) Erreur de msgsnd - 2");
                              exit(1);
                            }
                            kill(reponse.type, SIGUSR1);

                            sleep(1);
                          }
                          i++;
                        }

                      break;

      case ACCEPT_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete ACCEPT_USER reçue de %d\n",getpid(),m.expediteur);
                      i = 0;
                      
                      while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                      {
                        i++;
                      }
                      
                      if(tab->connexions[i].pidFenetre == m.expediteur)
                      {
                        j = 0;
                        while(j < 5 && tab->connexions[i].autres[j] != 0) 
                        {
                          j++;
                        }
                      
                        if(tab->connexions[i].autres[j] == 0)
                        {
                          n = 0;

                          while(n < 6 && strcmp(tab->connexions[n].nom, m.data1) != 0)
                          {
                            n++;
                          }

                          if(strcmp(tab->connexions[n].nom, m.data1) == 0)
                          {
                            tab->connexions[i].autres[j] = tab->connexions[n].pidFenetre;
                          }
                        }

                      }

                      break;

      case REFUSE_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete REFUSE_USER reçue de %d\n",getpid(),m.expediteur);
                      i = 0;
                      
                      while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                      {
                        i++;
                      }
                      
                      if(tab->connexions[i].pidFenetre == m.expediteur)
                      {
                        n = 0;

                        while(n < 6 && strcmp(tab->connexions[n].nom, m.data1) != 0)
                        {
                          n++;
                        }

                        if(strcmp(tab->connexions[n].nom, m.data1) == 0)
                        {
                          j = 0;
                          while(j < 5 && tab->connexions[i].autres[j] != tab->connexions[n].pidFenetre) 
                          {
                            j++;
                          }
                      
                          if(tab->connexions[i].autres[j] == tab->connexions[n].pidFenetre)
                          {
                            tab->connexions[i].autres[j] = 0;
                          }
                        }

                      }
                      break;

      case SEND :  
                      fprintf(stderr,"(SERVEUR %d) Requete SEND reçue de %d\n",getpid(),m.expediteur);
                      i = 0;
                      while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur)
                      {
                        i++;
                      }

                      if(tab->connexions[i].pidFenetre == m.expediteur)
                      {
                        reponse.expediteur = 1;
                        reponse.requete = SEND;
                        strcpy(reponse.texte, m.texte);
                        strcpy(reponse.data1, tab->connexions[i].nom);

                        i = 0;
                        while(i < 6)
                        {
                          if(tab->connexions[i].pidFenetre != 0)
                          {
                            j = 0;
                            while(j < 5) 
                            {
                              if(tab->connexions[i].autres[j] == m.expediteur)
                              {
                                reponse.type = tab->connexions[i].pidFenetre;
                                if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                                {
                                  msgctl(idQ, IPC_RMID, NULL);
                                  perror("(SERVEUR) Erreur de msgsnd - 2");
                                  exit(1);
                                }
                                kill(reponse.type, SIGUSR1);

                              }

                              j++;
                            }
                          }

                          i++;
                        }
                      }


                      
                      break; 

      case UPDATE_PUB :
                      fprintf(stderr,"(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);
                      break;

      case CONSULT :
                      fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case MODIF1 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF1 reçue de %d\n",getpid(),m.expediteur);
                      break;

      case MODIF2 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF2 reçue de %d\n",getpid(),m.expediteur);
                      break;

      case LOGIN_ADMIN :
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN_ADMIN reçue de %d\n",getpid(),m.expediteur);
                      break;

      case LOGOUT_ADMIN :
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT_ADMIN reçue de %d\n",getpid(),m.expediteur);
                      break;

      case NEW_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_USER reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
                      break;

      case DELETE_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete DELETE_USER reçue de %d : --%s--\n",getpid(),m.expediteur,m.data1);
                      break;

      case NEW_PUB :
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
                      break;
    }
    afficheTab();
  }
}

void afficheTab()
{
  fprintf(stderr,"Pid Serveur 1 : %d\n",tab->pidServeur1);
  fprintf(stderr,"Pid Serveur 2 : %d\n",tab->pidServeur2);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid Admin     : %d\n",tab->pidAdmin);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d %6d %6d %6d %6d - %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].autres[0],
                                                      tab->connexions[i].autres[1],
                                                      tab->connexions[i].autres[2],
                                                      tab->connexions[i].autres[3],
                                                      tab->connexions[i].autres[4],
                                                      tab->connexions[i].pidModification);
  fprintf(stderr,"\n");
}

void handlerSIGINT(int sig)
{
  fprintf(stderr, "Reception d un signal %d\n", sig);
  msgctl(idQ, IPC_RMID, NULL);
  mysql_close(connexion);
  exit(0);
}

