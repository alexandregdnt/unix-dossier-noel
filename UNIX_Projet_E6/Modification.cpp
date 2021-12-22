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
int semDispo = 1;

int main()
{
  char nom[40];

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(MODIFICATION %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE, 0)) == -1)
  {
    perror("(MODIFICATION) Erreur de msgget");
    exit(1);
  }

  // Recuperation de l'identifiant du sémaphore
  if ((idSem = semget(CLE,0,0)) == -1)
  {
    perror("Erreur de semget");
    exit(1);
  }

  MESSAGE m;
  // Lecture de la requête MODIF1
  fprintf(stderr,"(MODIFICATION %d) Lecture requete MODIF1\n",getpid());
  if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
  {
    perror("(MODIFICATION) Erreur de msgrcv");
    exit(1);
  }

  // Tentative de prise non bloquante du semaphore 0 (au cas où un autre utilisateut est déjà en train de modifier)
  operations[0].sem_num = 0;
  operations[0].sem_op = -1;
  operations[0].sem_flg = IPC_NOWAIT;

  if (semop(idSem,operations,1) == -1)
    semDispo = 0;


  // Connexion à la base de donnée
  MYSQL *connexion = mysql_init(NULL);
  fprintf(stderr,"(MODIFICATION %d) Connexion à la BD\n",getpid());
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(MODIFICATION) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Recherche des infos dans la base de données
  fprintf(stderr,"(MODIFICATION %d) Consultation en BD pour --%s--\n",getpid(),m.data1);
  strcpy(nom,m.data1);
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  char requete[200];
  sprintf(requete, "select motdepasse, gsm, email from UNIX_FINAL where nom='%s';",m.data1);
  mysql_query(connexion,requete);
  resultat = mysql_store_result(connexion);
  tuple = mysql_fetch_row(resultat); // user existe forcement

  // Construction et envoi de la reponse
  fprintf(stderr,"(MODIFICATION %d) Envoi de la reponse\n",getpid());
  m.type = m.expediteur;
  m.expediteur = getpid();
  m.requete = MODIF1;
  fprintf(stderr,"%d Envoi de la reponse\n",m.type);
  if(semDispo == 1)
  {
    strcpy(m.data1, tuple[0]);
    strcpy(m.data2, tuple[1]);
    strcpy(m.texte, tuple[2]);
  }
  else
  {
    strcpy(m.data1, "KO");
    strcpy(m.data2, "KO");
    strcpy(m.texte, "KO");
  }
  

  if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    msgctl(idQ, IPC_RMID, NULL);
    perror("(MODIFICATION) Erreur de msgsnd");
    exit(1);
  }
  kill(m.type, SIGUSR1);

  if(semDispo == 0)
    exit(0);
  
  // Attente de la requête MODIF2
  fprintf(stderr,"(MODIFICATION %d) Attente requete MODIF2...\n",getpid());
  if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
  {
    perror("(MODIFICATION) Erreur de msgrcv");
    exit(1);
  }

  // Mise à jour base de données
  fprintf(stderr,"(MODIFICATION %d) Modification en base de données pour --%s--\n",getpid(),nom);
  sprintf(requete, "UPDATE UNIX_FINAL SET motdepasse = '%s' WHERE nom = '%s'", m.data1, nom);
  mysql_query(connexion,requete);
  sprintf(requete, "UPDATE UNIX_FINAL SET gsm = '%s' WHERE nom = '%s'", m.data2, nom);
  mysql_query(connexion,requete);
  sprintf(requete, "UPDATE UNIX_FINAL SET email = '%s' WHERE nom = '%s'", m.texte, nom);
  mysql_query(connexion,requete);

  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  fprintf(stderr,"(MODIFICATION %d) Libération du sémaphore 0\n",getpid());
  if(semDispo == 1)
  {
    operations[0].sem_num = 0;
    operations[0].sem_op = +1;
    operations[0].sem_flg = 0;
    if (semop(idSem,operations,1) == -1)
      perror("(MODIFICATION) Erreur de semop (2)");
  }
  
  exit(0);
}

