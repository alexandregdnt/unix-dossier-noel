#include "windowadmin.h"
#include "ui_windowadmin.h"
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

extern int idQ;
MESSAGE message;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowAdmin::WindowAdmin(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowAdmin)
{
    ui->setupUi(this);
}

WindowAdmin::~WindowAdmin()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowAdmin::getNom()
{
  strcpy(nom,ui->lineEditNom->text().toStdString().c_str());
  return nom;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowAdmin::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setTexte(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditTexte->clear();
    return;
  }
  ui->lineEditTexte->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowAdmin::getTexte()
{
  strcpy(texte,ui->lineEditTexte->text().toStdString().c_str());
  return texte;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setNbSecondes(int n)
{
  char Text[10];
  sprintf(Text,"%d",n);
  ui->lineEditNbSecondes->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowAdmin::getNbSecondes()
{
  char tmp[10];
  strcpy(tmp,ui->lineEditNbSecondes->text().toStdString().c_str());
  return atoi(tmp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::on_pushButtonAjouterUtilisateur_clicked()
{
  message.type = 1;
  message.expediteur = getpid();
  message.requete = NEW_USER;
  strcpy(message.data1, getNom());
  strcpy(message.data2, getMotDePasse());
  if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    msgctl(idQ, IPC_RMID, NULL);
    perror("(ADMIN) Erreur de msgsnd - 2");
    exit(1);
  }

  setNom("");
  setMotDePasse("");
}

void WindowAdmin::on_pushButtonSupprimerUtilisateur_clicked()
{
  message.type = 1;
  message.expediteur = getpid();
  message.requete = DELETE_USER;
  strcpy(message.data1, getNom());
  if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    msgctl(idQ, IPC_RMID, NULL);
    perror("(ADMIN) Erreur de msgsnd - 2");
    exit(1);
  }

  setNom("");
  setMotDePasse("");
}

void WindowAdmin::on_pushButtonAjouterPublicite_clicked()
{
  message.type = 1;
  message.expediteur = getpid();
  message.requete = NEW_PUB;
  sprintf(message.data1, "%d", getNbSecondes());
  strcpy(message.texte, getTexte());
  if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    msgctl(idQ, IPC_RMID, NULL);
    perror("(ADMIN) Erreur de msgsnd - 3");
    exit(1);
  }

  setNbSecondes(0);
  setTexte("");
}

void WindowAdmin::on_pushButtonQuitter_clicked()
{
  message.type = 1;
  message.expediteur = getpid();
  message.requete = LOGOUT_ADMIN;
  if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    msgctl(idQ, IPC_RMID, NULL);
    perror("(ADMIN) Erreur de msgsnd - 4");
    exit(1);
  }
  exit(0);
}
