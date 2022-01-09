#include "windowadmin.h"
#include <QApplication>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int idQ;

int main(int argc, char *argv[])
{
    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(ADMINISTRATEUR %d) Recuperation de l'id de la file de messages\n",getpid());
    if ((idQ = msgget(CLE, 0)) == -1)
    {
      perror("(ADMIN) Erreur de msgget - 1");
      exit(1);
    }

    // Envoi d'une requete de connexion au serveur
    MESSAGE message;

    message.type = 1;
    message.expediteur = getpid();
    message.requete = LOGIN_ADMIN;

    if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      msgctl(idQ, IPC_RMID, NULL);
      perror("(ADMIN) Erreur de msgsnd - 1");
      exit(1);
    }

    // Attente de la réponse
    fprintf(stderr,"(ADMINISTRATEUR %d) Attente reponse\n",getpid());
    if (msgrcv(idQ, &message, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
    {
      perror("(ADMIN) Erreur de msgrcv");
      exit(1);
    }

    if(strcmp(message.data1, "KO") == 0)
    {
      exit(0);
    }

    QApplication a(argc, argv);
    WindowAdmin w;
    w.show();
    return a.exec();
}
