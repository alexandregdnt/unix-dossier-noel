#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include "dialogmodification.h"
#include <unistd.h>
#include <time.h>

extern WindowClient *w;

#include "protocole.h"

int idQ, idShm;
#define TIME_OUT 30 // 120
int timeOut = TIME_OUT;
MESSAGE message;
struct sigaction Action;

void handlerSIGUSR1(int sig);
void handlerSIGALARM(int sig);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowClient::WindowClient(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowClient)
{
    ui->setupUi(this);
    logoutOK();

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());
    if ((idQ = msgget(CLE, 0)) == -1)
    {
      perror("(CLIENT) Erreur de msgget - 1");
      exit(1);
    }

    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());

    // Attachement à la mémoire partagée

    // Armement des signaux
    Action.sa_handler = handlerSIGUSR1;
    sigemptyset(&Action.sa_mask);
    Action.sa_flags = 0;
    sigaction(SIGUSR1, &Action, NULL);

    Action.sa_handler = handlerSIGALARM;
    sigemptyset(&Action.sa_mask);
    Action.sa_flags = 0;
    sigaction(SIGALRM, &Action, NULL);

    // Envoi d'une requete de connexion au serveur
    message.type = 1;
    message.expediteur = getpid();
    message.requete = CONNECT;

    if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      msgctl(idQ, IPC_RMID, NULL);
      perror("(CLIENT) Erreur de msgsnd - 1");
      exit(1);
    }
}

WindowClient::~WindowClient()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNom()
{
  strcpy(connectes[0],ui->lineEditNom->text().toStdString().c_str());
  return connectes[0];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditPublicite->clear();
    return;
  }
  ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setTimeOut(int nb)
{
  ui->lcdNumberTimeOut->display(nb);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setAEnvoyer(const char* Text)
{
  //fprintf(stderr,"---%s---\n",Text);
  if (strlen(Text) == 0 )
  {
    ui->lineEditAEnvoyer->clear();
    return;
  }
  ui->lineEditAEnvoyer->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getAEnvoyer()
{
  strcpy(aEnvoyer,ui->lineEditAEnvoyer->text().toStdString().c_str());
  return aEnvoyer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPersonneConnectee(int i,const char* Text)
{
  if (strlen(Text) == 0 )
  {
    switch(i)
    {
        case 1 : ui->lineEditConnecte1->clear(); break;
        case 2 : ui->lineEditConnecte2->clear(); break;
        case 3 : ui->lineEditConnecte3->clear(); break;
        case 4 : ui->lineEditConnecte4->clear(); break;
        case 5 : ui->lineEditConnecte5->clear(); break;
    }
    return;
  }
  switch(i)
  {
      case 1 : ui->lineEditConnecte1->setText(Text); break;
      case 2 : ui->lineEditConnecte2->setText(Text); break;
      case 3 : ui->lineEditConnecte3->setText(Text); break;
      case 4 : ui->lineEditConnecte4->setText(Text); break;
      case 5 : ui->lineEditConnecte5->setText(Text); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getPersonneConnectee(int i)
{
  QLineEdit *tmp;
  switch(i)
  {
    case 1 : tmp = ui->lineEditConnecte1; break;
    case 2 : tmp = ui->lineEditConnecte2; break;
    case 3 : tmp = ui->lineEditConnecte3; break;
    case 4 : tmp = ui->lineEditConnecte4; break;
    case 5 : tmp = ui->lineEditConnecte5; break;
    default : return NULL;
  }

  strcpy(connectes[i],tmp->text().toStdString().c_str());
  return connectes[i];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteMessage(const char* personne,const char* message)
{
  // Choix de la couleur en fonction de la position
  int i=1;
  bool trouve=false;
  while (i<=5 && !trouve)
  {
      if (getPersonneConnectee(i) != NULL && strcmp(getPersonneConnectee(i),personne) == 0) trouve = true;
      else i++;
  }
  char couleur[40];
  if (trouve)
  {
      switch(i)
      {
        case 1 : strcpy(couleur,"<font color=\"red\">"); break;
        case 2 : strcpy(couleur,"<font color=\"blue\">"); break;
        case 3 : strcpy(couleur,"<font color=\"green\">"); break;
        case 4 : strcpy(couleur,"<font color=\"darkcyan\">"); break;
        case 5 : strcpy(couleur,"<font color=\"orange\">"); break;
      }
  }
  else strcpy(couleur,"<font color=\"black\">");
  if (strcmp(getNom(),personne) == 0) strcpy(couleur,"<font color=\"purple\">");

  // ajout du message dans la conversation
  char buffer[300];
  sprintf(buffer,"%s(%s)</font> %s",couleur,personne,message);
  ui->textEditConversations->append(buffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNomRenseignements(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNomRenseignements->clear();
    return;
  }
  ui->lineEditNomRenseignements->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNomRenseignements()
{
  strcpy(nomR,ui->lineEditNomRenseignements->text().toStdString().c_str());
  return nomR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setGsm(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditGsm->clear();
    return;
  }
  ui->lineEditGsm->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setEmail(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditEmail->clear();
    return;
  }
  ui->lineEditEmail->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setCheckbox(int i,bool b)
{
  QCheckBox *tmp;
  switch(i)
  {
    case 1 : tmp = ui->checkBox1; break;
    case 2 : tmp = ui->checkBox2; break;
    case 3 : tmp = ui->checkBox3; break;
    case 4 : tmp = ui->checkBox4; break;
    case 5 : tmp = ui->checkBox5; break;
    default : return;
  }
  tmp->setChecked(b);
  if (b) tmp->setText("Accepté");
  else tmp->setText("Refusé");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::loginOK()
{
  ui->pushButtonLogin->setEnabled(false);
  ui->pushButtonLogout->setEnabled(true);
  ui->lineEditNom->setReadOnly(true);
  ui->lineEditMotDePasse->setReadOnly(true);
  ui->pushButtonEnvoyer->setEnabled(true);
  ui->pushButtonConsulter->setEnabled(true);
  ui->pushButtonModifier->setEnabled(true);
  ui->checkBox1->setEnabled(true);
  ui->checkBox2->setEnabled(true);
  ui->checkBox3->setEnabled(true);
  ui->checkBox4->setEnabled(true);
  ui->checkBox5->setEnabled(true);
  ui->lineEditAEnvoyer->setEnabled(true);
  ui->lineEditNomRenseignements->setEnabled(true);
  setTimeOut(TIME_OUT);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::logoutOK()
{
  ui->pushButtonLogin->setEnabled(true);
  ui->pushButtonLogout->setEnabled(false);
  ui->lineEditNom->setReadOnly(false);
  ui->lineEditNom->setText("");
  ui->lineEditMotDePasse->setReadOnly(false);
  ui->lineEditMotDePasse->setText("");
  ui->pushButtonEnvoyer->setEnabled(false);
  ui->pushButtonConsulter->setEnabled(false);
  ui->pushButtonModifier->setEnabled(false);
  for (int i=1 ; i<=5 ; i++)
  {
      setCheckbox(i,false);
      setPersonneConnectee(i,"");
  }
  ui->checkBox1->setEnabled(false);
  ui->checkBox2->setEnabled(false);
  ui->checkBox3->setEnabled(false);
  ui->checkBox4->setEnabled(false);
  ui->checkBox5->setEnabled(false);
  setNomRenseignements("");
  setGsm("");
  setEmail("");
  ui->textEditConversations->clear();
  setAEnvoyer("");
  ui->lineEditAEnvoyer->setEnabled(false);
  ui->lineEditNomRenseignements->setEnabled(false);
  setTimeOut(TIME_OUT);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked()
{
  message.type = 1;
  message.expediteur = getpid();
  message.requete = LOGIN;
  strcpy(message.data1, getNom());
  strcpy(message.data2, getMotDePasse());

  if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    msgctl(idQ, IPC_RMID, NULL);
    perror("(CLIENT) Erreur de msgsnd - 1");
    exit(1);
  }

}

void WindowClient::on_pushButtonLogout_clicked()
{
  message.type = 1;
  message.expediteur = getpid();
  message.requete = LOGOUT;

  if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    msgctl(idQ, IPC_RMID, NULL);
    perror("(CLIENT) Erreur de msgsnd - 2");
    exit(1);
  }

  setNom("");
  setMotDePasse("");

  logoutOK();

  timeOut = TIME_OUT +1;
}

void WindowClient::on_pushButtonQuitter_clicked()
{
  if(strcmp(getNom(), "") != 0 && strcmp(getMotDePasse(), "") != 0)
  {
    WindowClient::on_pushButtonLogout_clicked();
  }

  message.type = 1;
  message.expediteur = getpid();
  message.requete = DECONNECT;

  if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    msgctl(idQ, IPC_RMID, NULL);
    perror("(CLIENT) Erreur de msgsnd - 3");
    exit(1);
  }

  exit(0);
}

void WindowClient::on_pushButtonEnvoyer_clicked()
{
  message.type = 1;
  message.expediteur = getpid();
  message.requete = SEND;
  strcpy(message.texte, getAEnvoyer());

  if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    msgctl(idQ, IPC_RMID, NULL);
    perror("(CLIENT) Erreur de msgsnd - 4");
    exit(1);
  }

    timeOut = TIME_OUT;
    alarm(1);
}

void WindowClient::on_pushButtonConsulter_clicked()
{
    timeOut = TIME_OUT;
    alarm(1);
}

void WindowClient::on_pushButtonModifier_clicked()
{
  // TO DO
  // Envoi d'une requete MODIF1 au serveur
  MESSAGE m;
  // ...

  // Attente d'une reponse en provenance de Modification
  fprintf(stderr,"(CLIENT %d) Attente reponse MODIF1\n",getpid());
  // ...

  // Verification si la modification est possible
  if (strcmp(m.data1,"KO") == 0 && strcmp(m.data2,"KO") == 0 && strcmp(m.texte,"KO") == 0)
  {
    QMessageBox::critical(w,"Problème...","Modification déjà en cours...");
    return;
  }

  // Modification des données par utilisateur
  DialogModification dialogue(this,getNom(),m.data1,m.data2,m.texte);
  dialogue.exec();
  char motDePasse[40];
  char gsm[40];
  char email[40];
  strcpy(motDePasse,dialogue.getMotDePasse());
  strcpy(gsm,dialogue.getGsm());
  strcpy(email,dialogue.getEmail());

  // Envoi des données modifiées au serveur
  // ...

    timeOut = TIME_OUT;
    alarm(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les checkbox ///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_checkBox1_clicked(bool checked)
{
    message.type = 1;
    message.expediteur = getpid();
    strcpy(message.data1, w->getPersonneConnectee(1));

    if (checked)
    {
        ui->checkBox1->setText("Accepté");
        message.requete = ACCEPT_USER;
    }
    else
    {
        ui->checkBox1->setText("Refusé");
        message.requete = REFUSE_USER;
    }

    if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      msgctl(idQ, IPC_RMID, NULL);
      perror("(CLIENT) Erreur de msgsnd - 3");
      exit(1);
    }

    timeOut = TIME_OUT;
    alarm(1);
}

void WindowClient::on_checkBox2_clicked(bool checked)
{
    message.type = 1;
    message.expediteur = getpid();
    strcpy(message.data1, w->getPersonneConnectee(2));

    if (checked)
    {
        ui->checkBox2->setText("Accepté");
        message.requete = ACCEPT_USER;
    }
    else
    {
        ui->checkBox2->setText("Refusé");
        message.requete = REFUSE_USER;
    }

    if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      msgctl(idQ, IPC_RMID, NULL);
      perror("(CLIENT) Erreur de msgsnd - 4");
      exit(1);
    }

    timeOut = TIME_OUT;
    alarm(1);
}

void WindowClient::on_checkBox3_clicked(bool checked)
{
    message.type = 1;
    message.expediteur = getpid();
    strcpy(message.data1, w->getPersonneConnectee(3));

    if (checked)
    {
       ui->checkBox3->setText("Accepté");
       message.requete = ACCEPT_USER;
    }
    else
    {
        ui->checkBox3->setText("Refusé");
        message.requete = REFUSE_USER;
    }

    if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      msgctl(idQ, IPC_RMID, NULL);
      perror("(CLIENT) Erreur de msgsnd - 5");
      exit(1);
    }

    timeOut = TIME_OUT;
    alarm(1);
}

void WindowClient::on_checkBox4_clicked(bool checked)
{
    message.type = 1;
    message.expediteur = getpid();
    strcpy(message.data1, w->getPersonneConnectee(4));

    if (checked)
    {
        ui->checkBox4->setText("Accepté");
        message.requete = ACCEPT_USER;
    }
    else
    {
        ui->checkBox4->setText("Refusé");
        message.requete = REFUSE_USER;
    }

    if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      msgctl(idQ, IPC_RMID, NULL);
      perror("(CLIENT) Erreur de msgsnd - 6");
      exit(1);
    }

    timeOut = TIME_OUT;
    alarm(1);
}

void WindowClient::on_checkBox5_clicked(bool checked)
{
    message.type = 1;
    message.expediteur = getpid();
    strcpy(message.data1, w->getPersonneConnectee(5));

    if (checked)
    {
        ui->checkBox5->setText("Accepté");
        message.requete = ACCEPT_USER;
    }
    else
    {
        ui->checkBox5->setText("Refusé");
        message.requete = REFUSE_USER;
    }

    if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      msgctl(idQ, IPC_RMID, NULL);
      perror("(CLIENT) Erreur de msgsnd - 7");
      exit(1);
    }

    timeOut = TIME_OUT;
    alarm(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int sig)
{
    MESSAGE m;
    int j;
    int rc;

    while((rc = msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), IPC_NOWAIT)) != -1)
    {
      
      switch(m.requete)
      {
        case LOGIN :
                    if (strcmp(m.data1,"OK") == 0)
                    {
                      fprintf(stderr,"(CLIENT %d) Login OK\n",getpid());
                      w->loginOK();
                      QMessageBox::information(w,"Login...","Login Ok ! Bienvenue...");
                      // ...

                      alarm(1);
                    }
                    else QMessageBox::critical(w,"Login...","Erreur de login...");
                    break;

        case ADD_USER :
                    j = 1;
                    while(j < 6 && strcmp(w->getPersonneConnectee(j), "") != 0)
                    {
                      j++;
                    }
                    
                    if(strcmp(w->getPersonneConnectee(j), "") == 0)
                    {
                      w->setPersonneConnectee(j, m.data1);
                    }

                    break;

        case REMOVE_USER :
                    j = 1;
                    while(j < 5 && strcmp(w->getPersonneConnectee(j), m.data1) != 0)
                    {
                      j++;
                    }
                    
                    if(strcmp(w->getPersonneConnectee(j), m.data1) == 0)
                    {
                      w->setPersonneConnectee(j, "");
                    }
                    break;

        case SEND :
                    w->ajouteMessage(m.data1, m.texte);
                    break;

        case CONSULT :
                    // TO
                    break;
      }

    }

}

void handlerSIGALARM(int sig) {
    if (timeOut > TIME_OUT) {
        timeOut = TIME_OUT;
    } else {
        if (timeOut > 0) {
            w->setTimeOut(timeOut);
            timeOut--;

            alarm(1);
        } else {
            w->WindowClient::on_pushButtonLogout_clicked();
            timeOut = TIME_OUT;
            w->setTimeOut(timeOut);
        }
    }
}
