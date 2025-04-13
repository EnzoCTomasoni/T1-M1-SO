#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_NOME 50

//dados
typedef struct {
    int id;
    char nome[MAX_NOME];
} Registro;

//requisição
typedef struct {
    char operacao[10];
    Registro registro;
} Requisicao;

//limpa entrada
void limpar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    int escolha;
    Requisicao req;
    int pipe;

    //cria a pipe
    mkfifo("/tmp/pipeEnvio", 0666);
    mkfifo("/tmp/pipeResposta", 0666);

    //loop do menu
    while (1) {
        //mostra as opções
        printf("\n--- MENU ---\n");
        printf("[1] Inserir registro\n");
        printf("[2] Deletar registro\n");
        printf("[3] Atualizar Registro\n");
        printf("[4] Consultar Registro\n");
        printf("[0] Sair\n");
        printf("Escolha: ");
        scanf("%d", &escolha);
        limpar_buffer();

        //sair
        if (escolha == 0) {
            printf("Encerrando cliente.\n");
            break;
        }

        //abre a pipe
        pipe = open("/tmp/pipeEnvio", O_WRONLY); // Corrigido

        //inserir
        if (escolha == 1) {
            //atribui o tipo insert a operação
            strcpy(req.operacao, "INSERT");
            //pede o nome
            printf("Nome: ");
            fgets(req.registro.nome, MAX_NOME, stdin);
            //passa a string com o \n removido como nome
            req.registro.nome[strcspn(req.registro.nome, "\n")] = '\0';
            //passa como 0, para o servidor atribuir
            req.registro.id = 0;

        //deletar
        } else if (escolha == 2) {
            //atribui o tipo delete a operação
            strcpy(req.operacao, "DELETE");
            //pede o ID
            printf("ID a deletar: ");
            scanf("%d", &req.registro.id);
            limpar_buffer();
            //passa o nome vazio
            strcpy(req.registro.nome, ""); // não necessário, mas evita lixo

        //update
        } else if (escolha == 3) {
            //atribui o tipo update a operação
            strcpy(req.operacao, "UPDATE");
            //pede o ID
            printf("ID: ");
            scanf("%d", &req.registro.id);
            limpar_buffer();
            //pede o novo nome
            printf("Novo nome: ");
            fgets(req.registro.nome, MAX_NOME, stdin);
            req.registro.nome[strcspn(req.registro.nome, "\n")] = '\0';

        //select
        } else if (escolha == 4) {
            //atribui o tipo select a operação
            strcpy(req.operacao, "SELECT");
            //pede o ID
            printf("ID: ");
            scanf("%d", &req.registro.id);
            limpar_buffer();
        
        } else {
            printf("Opção inválida.\n");
            close(pipe);
            continue;
        }

        //escreve a requisição na pipe
        write(pipe, &req, sizeof(Requisicao));
        close(pipe);
        printf("Requisição enviada.\n");

        //caso seja select
        if (strcmp(req.operacao, "SELECT") == 0) {
            //abre a pipe de resposta
            int pipe_resposta = open("/tmp/pipeResposta", O_RDONLY);
            //espera uma resposta
            char resposta[MAX_NOME + 50];
            //caso o conteudo não seja vazio o lê e depois o torna vazio
            int conteudoResposta = read(pipe_resposta, resposta, sizeof(resposta));
            if (conteudoResposta > 0) {
                resposta[conteudoResposta] = '\0';
                printf("Resposta do servidor: %s\n", resposta);
            } else {
                printf("Erro ao ler resposta do servidor.\n");
            }
            close(pipe_resposta);
        }
    }

    return 0;
}
