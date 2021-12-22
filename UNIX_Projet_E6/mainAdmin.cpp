#include "windowadmin.h"
#include <QApplication>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

int idQ;

int main(int argc, char *argv[])
{
    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(ADMINISTRATEUR %d) Recuperation de l'id de la file de messages\n",getpid());
    if ((idQ = msgget(CLE, 0)) == -1)
    {
        perror("(ADMINISTRATEUR) Erreur de msgget");
        exit(1);
    }
    printf("idQ: %d \n", idQ);

    // Envoi d'une requete de connexion au serveur
    MESSAGE message;
    message.type = 14;
    message.expediteur = getpid();
    message.requete = LOGIN_ADMIN;
    if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
        msgctl(idQ, IPC_RMID, NULL);
        perror("(ADMINISTRATEUR) Erreur de msgsnd");
        exit(1);
    }

    // Attente de la réponse
    while (1) 
    {
        fprintf(stderr,"(ADMINISTRATEUR %d) Attente reponse\n",getpid());

        if (msgrcv(idQ,&message,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
        {
            perror("(ADMINISTRATEUR) Erreur de msgrcv - 1");
            msgctl(idQ,IPC_RMID,NULL);
            exit(1);
        }
        printf("(ADMINISTRATEUR Reponse du serveur: %s \n", message.data1);
        if (strcmp(message.data1, "OK") == 0) {
            printf("OK -> On ouvre admin \n");
        }
        if (strcmp(message.data1, "KO") == 0) {
            printf("KO -> On ferme admin \n");
            exit(1);
        }
    }

    QApplication a(argc, argv);
    WindowAdmin w;
    w.show();
    return a.exec();
}
