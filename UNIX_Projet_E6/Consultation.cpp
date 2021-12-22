#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <mysql.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "protocole.h"

int idQ,idSem;
struct sembuf operations[1];

int main()
{
  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(CONSULTATION %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE, 0)) == -1)
  {
    perror("(CONSULTATION) Erreur de msgget");
    exit(1);
  }

  // Recuperation de l'identifiant du sémaphore
  if ((idSem = semget(CLE,0,0)) == -1)
  {
    perror("Erreur de semget");
    exit(1);
  }

  MESSAGE m;
  // Lecture de la requête CONSULT
  fprintf(stderr,"(CONSULTATION %d) Lecture requete CONSULT\n",getpid());
  if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
  {
    perror("(CONSULTATION) Erreur de msgrcv");
    exit(1);
  }

  // Tentative de prise bloquante du semaphore 0
  fprintf(stderr,"(CONSULTATION %d) Prise bloquante du sémaphore 0\n",getpid());
  operations[0].sem_num = 0;
  operations[0].sem_op = -1;
  operations[0].sem_flg = 0;

  if (semop(idSem,operations,1) == -1)
    perror("(CONSULTATION) Erreur de semop (1)");

  // Connexion à la base de donnée
  MYSQL *connexion = mysql_init(NULL);
  fprintf(stderr,"(CONSULTATION %d) Connexion à la BD\n",getpid());
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(CONSULTATION) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Recherche des infos dans la base de données
  fprintf(stderr,"(CONSULTATION %d) Consultation en BD (%s)\n",getpid(),m.data1);
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  char requete[200];
  sprintf(requete, "select gsm, email from UNIX_FINAL where nom='%s';",m.data1);
  mysql_query(connexion,requete),
  resultat = mysql_store_result(connexion);
  if((tuple = mysql_fetch_row(resultat)) != NULL)
  {
    strcpy(m.data1, "OK");
    strcpy(m.data2, tuple[0]);
    strcpy(m.texte, tuple[1]);
  }
  else
  {
    strcpy(m.data1, "KO");
  }
  	
  // Construction et envoi de la reponse
  fprintf(stderr,"(CONSULTATION %d) Envoi de la reponse\n",getpid());
  m.type = m.expediteur;
  m.expediteur = getpid();
  m.requete = CONSULT;
  if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    msgctl(idQ, IPC_RMID, NULL);
    perror("(CONSULTATION) Erreur de msgsnd");
    exit(1);
  }
  kill(m.type, SIGUSR1);

  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  fprintf(stderr,"(CONSULTATION %d) Libération du sémaphore 0\n",getpid());
  operations[0].sem_num = 0;
  operations[0].sem_op = +1;
  operations[0].sem_flg = 0;

  if (semop(idSem,operations,1) == -1)
    perror("(CONSULTATION) Erreur de semop (2)");

  exit(0);
}