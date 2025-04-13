#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_NOME 50

//dados
typedef struct {
    int id;
    char nome[MAX_NOME];
} Registro;

//requisição
typedef struct {
    char operacao[10]; // "INSERT", "DELETE", etc.
    Registro registro;
} Requisicao;

//inicializa o mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Função para gerar automaticamente o próximo ID baseado no banco.txt
int gerar_proximo_id() {
    //abre o banco
    FILE* db = fopen("banco.txt", "r");
    if (!db) return 1; // Se o arquivo não existir ainda, começa do ID 1

    int id, max_id = 0;
    char nome[MAX_NOME];
    //lê as linhas, remove o nome, e atualiza o max_id com o maior id
    while (fscanf(db, "%d %s", &id, nome) != EOF) {
        if (id > max_id) max_id = id;
    }

    fclose(db);
    return max_id + 1;
}

void* processar_requisicao(void* arg) {
    Requisicao* req = (Requisicao*)arg;

    pthread_mutex_lock(&mutex);

    if (strcmp(req->operacao, "INSERT") == 0) {
        //define o novo id
        req->registro.id = gerar_proximo_id();
        //abre o banco
        FILE* db = fopen("banco.txt", "a");
        //escreve a nova linha
        fprintf(db, "%d %s\n", req->registro.id, req->registro.nome);
        fclose(db);
        //confirma
        printf("INSERT COM SUCESSO!\n");

    } else if (strcmp(req->operacao, "DELETE") == 0) {
        //abre o banco e um temporário
        FILE* db = fopen("banco.txt", "r");
        FILE* temp = fopen("temp.txt", "w");

        int id;
        char nome[MAX_NOME];
        int encontrado = 0;

        //corre o banco de dados copiando as linhas
        while (fscanf(db, "%d %s", &id, nome) != EOF) {
            if (id != req->registro.id) {
                fprintf(temp, "%d %s\n", id, nome);
            } else {
                //caso tenha o id ela não é copiada
                encontrado = 1;
            }
        }

        //fecha os arquivos
        fclose(db);
        fclose(temp);
        //substitui o velho pelo novo
        remove("banco.txt");
        rename("temp.txt", "banco.txt");

        //confirma
        if (encontrado) {
            printf("DELETE COM SUCESSO!\n");
        } else {
            printf("DELETE SEM SUCESSO!\n");
        }
    }

    else if (strcmp(req->operacao, "UPDATE") == 0) {
        //abre o banco e um temporario
        FILE* db = fopen("banco.txt", "r");
        FILE* temp = fopen("temp.txt", "w");

        int id;
        char nome[MAX_NOME];

        //copia todas as linhas
        while (fscanf(db, "%d %[^\n]", &id, nome) != EOF) {
            if (id == req->registro.id) {
                //atualiza o ID selecionado
                fprintf(temp, "%d %s\n", id, req->registro.nome);
                printf("UPDATE COM SUCESSO!\n");
            } else {
                fprintf(temp, "%d %s\n", id, nome);
            }
        }
        fclose(db);
        fclose(temp);
        remove("banco.txt");
        rename("temp.txt", "banco.txt");

    } else if (strcmp(req->operacao, "SELECT") == 0) {
        //abre o banco
        FILE* db = fopen("banco.txt", "r");
        int id;
        char nome[MAX_NOME];
        int encontrado = 0;

        //corre as linhas procurando o ID
        while (fscanf(db, "%d %s", &id, nome) != EOF) {
            if (id == req->registro.id) {
                encontrado = 1;
                break;
            }
        }

        fclose(db);

        //envia resposta para a pipe resposta
        int resp_fd = open("/tmp/pipeResposta", O_WRONLY);
        if (resp_fd >= 0) {
            if (encontrado) {
                //encontrou
                write(resp_fd, nome, MAX_NOME);
            } else {
                //não encontrou
                char notfound[] = "Registro não encontrado";
                write(resp_fd, notfound, sizeof(notfound));
            }
            close(resp_fd);
            printf("SELECT COM SUCESSO\n");
        } 

    }

    pthread_mutex_unlock(&mutex);
    free(req);
    return NULL;
}

int main() {
    Requisicao req;
    int pipe;

    //cria a pipe
    mkfifo("/tmp/pipeEnvio", 0666);
    mkfifo("/tmp/pipeResposta", 0666);
    printf("Servidor aguardando requisições...\n");

    //loop infinito
    while (1) {
        //abre a pipe
        pipe = open("/tmp/pipeEnvio", O_RDONLY);

        //lê a pipe enquanto existem requisições
        while (read(pipe, &req, sizeof(Requisicao)) > 0) {
            pthread_t tid;
            //aloca espaço para a requisição e insere em *nova para que seja preservada
            Requisicao* nova_req = malloc(sizeof(Requisicao));
            *nova_req = req;
            //cria a thread para processar, passando a nova requisição
            pthread_create(&tid, NULL, processar_requisicao, nova_req);
            //segura a thread
            pthread_join(tid, NULL);
        }

        //fecha a pipe
        close(pipe);
    }

    return 0;
}
