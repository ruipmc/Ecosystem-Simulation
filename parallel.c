#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<omp.h>

int GEN_PROC_RABBITS;
int GEN_PROC_FOXES;
int GEN_FOOD_FOXES;
int N_GEN;
int R;
int C;
int N;
int current_gen;
int num_threads;

typedef struct object_{
    char type;
    int num_gen;
    int num_food;
}object;

typedef struct pos_ {
  int x,y;
}pos;

object **world;
object **new_world;

// LOCK MATRIX - one lock per cell to avoid race conditions
omp_lock_t **cell_locks;

/* Initialize locks for each cell in the grid */
void init_locks() {
  int x, y;
  cell_locks = (omp_lock_t **)malloc(sizeof(omp_lock_t*) * R);
  for(x = 0; x < R; x++) {
    cell_locks[x] = (omp_lock_t *)malloc(C * sizeof(omp_lock_t));
    for(y = 0; y < C; y++) {
      omp_init_lock(&cell_locks[x][y]);
    }
  }
}

/* Destroy all locks */
void destroy_locks() {
  int x, y;
  for(x = 0; x < R; x++) {
    for(y = 0; y < C; y++) {
      omp_destroy_lock(&cell_locks[x][y]);
    }
    free(cell_locks[x]);
  }
  free(cell_locks);
}

/* Displays the current state of the world grid for the given generation */
void print_world() {
  int x,y;
  printf("Generation %d\n", current_gen);
  for(x = 0; x < C + 2; x++)
    printf("-");
  printf("\n");
  for(x = 0; x < R; x++) {
    printf("|");
    for(y = 0; y < C; y++) {
      printf("%c", world[x][y].type);
    }
    printf("|");
    printf("\n");
  }
  for(x = 0; x < C + 2; x++)
    printf("-");
  printf("\n");
  printf("\n");
}

/* Initializes all cells in the world grids as empty spaces */
void init_world(){
  int x,y;
  #pragma omp parallel for private(y) schedule(static)
  for(x = 0; x < R; x++) {
    for(y = 0; y < C; y++) {
      object empty;
      empty.type = ' ';
      empty.num_gen = 0;
      empty.num_food = 0;
      world[x][y] = empty;
      new_world[x][y] = empty;
    }
  }
}

/* Copies all rabbits from the current world to the new world */
void copy_rabbits(){
  int x,y;
  #pragma omp parallel for private(y) schedule(static)
  for(x = 0; x < R; x++) {
    for(y = 0; y < C; y++) {
      if(world[x][y].type == 'R'){
        new_world[x][y] = world[x][y];
      }
    }
  }
}

/* Resets cells in the new world that don't contain rocks */
void reset_new_world(){
  int x,y;
  #pragma omp parallel for private(y) schedule(static)
  for(x = 0; x < R; x++) {
    for(y = 0; y < C; y++) {
      if(new_world[x][y].type != '*'){
        new_world[x][y].type = ' ';
        new_world[x][y].num_gen = 0;
        new_world[x][y].num_food = 0;
      }
    }
  }
}

/* Fills the world with initial objects (rabbits, foxes, rocks) from input */
void fill_world(){
  int i, x, y;
  char name[7];
  for(i = 0; i < N; i++) {
    scanf("%s %d %d", name, &x, &y);
    object new_addition;
    if(strcmp(name, "RABBIT") == 0){
      new_addition.type='R';
      new_addition.num_gen=GEN_PROC_RABBITS;
    }else if(strcmp(name, "FOX") == 0){
      new_addition.type='F';
      new_addition.num_gen=GEN_PROC_FOXES;
      new_addition.num_food=GEN_FOOD_FOXES;
    }else if(strcmp(name, "ROCK") == 0){
      new_addition.type='*';
      new_world[x][y] = new_addition;
    }
    world[x][y] = new_addition;
  }
}

/* Checks if the given coordinates are within the world boundaries */
int is_inside(int x, int y) {
  if(x < 0 || x >= R)
    return 0;
  if(y < 0 || y >= C)
    return 0;
  return 1;
}

/* Moves a rabbit from its current position to an adjacent empty cell or reproduces */
void move_rabbit(int x, int y) {
  object current = world[x][y], *new;
  int p = 0, new_pos_index;
  pos free_pos[4], new_pos;
  
  //North
  if(is_inside(x - 1, y) && world[x - 1][y].type == ' ') {
    free_pos[p].x = x - 1;
    free_pos[p].y = y;
    p++;
  }
  //East
  if(is_inside(x, y + 1) && world[x][y+1].type == ' ') {
    free_pos[p].x = x;
    free_pos[p].y = y + 1;
    p++;
  }
  //South
  if(is_inside(x + 1, y) && world[x + 1][y].type == ' ') {
    free_pos[p].x = x + 1;
    free_pos[p].y = y;
    p++;
  }
  //West
  if(is_inside(x, y - 1) && world[x][y - 1].type == ' ') {
    free_pos[p].x = x;
    free_pos[p].y = y - 1;
    p++;
  }
  
  if(p == 0){
    free_pos[0].x = x;
    free_pos[0].y = y;
    p = 1;
    if(current.num_gen == 0) {
      current.num_gen = 1;
    }
  }
  else {
    if(current.num_gen == 0) {
      // LOCK: Protege a célula atual ao criar filho
      omp_set_lock(&cell_locks[x][y]);
      new = &new_world[x][y];
      new->type = current.type;
      new->num_gen = GEN_PROC_RABBITS;
      omp_unset_lock(&cell_locks[x][y]);
      
      current.num_gen = GEN_PROC_RABBITS + 1;
    }
  }
  
  new_pos_index = (x + y + current_gen) % p;
  new_pos = free_pos[new_pos_index];
  new = &new_world[new_pos.x][new_pos.y];
  
  // LOCK: Protege a célula de destino de conflitos (múltiplos coelhos tentando mover para mesma célula)
  omp_set_lock(&cell_locks[new_pos.x][new_pos.y]);
  {
    if(new->type == 'R'){
      // Resolve conflito: mantém o coelho mais jovem
      if(current.num_gen - 1 < new->num_gen)
        new->num_gen = current.num_gen - 1;
    }
    else {
      new->type = current.type;
      new->num_gen = current.num_gen - 1;
    }
  }
  omp_unset_lock(&cell_locks[new_pos.x][new_pos.y]);
}

/* Moves a fox to hunt a rabbit or moves to an empty cell, handling reproduction and starvation */
void move_fox(int x, int y) {
  object current = world[x][y], *new;
  int p = 0, new_pos_index;
  pos free_pos[4], new_pos;
  
  // Procura coelhos adjacentes (prioridade)
  //North
  if(is_inside(x - 1, y) && world[x - 1][y].type == 'R') {
    free_pos[p].x = x - 1;
    free_pos[p].y = y;
    p++;
  }
  //East
  if(is_inside(x, y + 1) && world[x][y+1].type == 'R') {
    free_pos[p].x = x;
    free_pos[p].y = y + 1;
    p++;
  }
  //South
  if(is_inside(x + 1, y) && world[x + 1][y].type == 'R') {
    free_pos[p].x = x + 1;
    free_pos[p].y = y;
    p++;
  }
  //West
  if(is_inside(x, y - 1) && world[x][y - 1].type == 'R') {
    free_pos[p].x = x;
    free_pos[p].y = y - 1;
    p++;
  }
  
  if(p == 0){
    // Morre de fome
    if(current.num_food == 1)
      return;
    
    // Procura células vazias
    //North
    if(is_inside(x - 1, y) && world[x - 1][y].type == ' ') {
      free_pos[p].x = x - 1;
      free_pos[p].y = y;
      p++;
    }
    //East
    if(is_inside(x, y + 1) && world[x][y+1].type == ' ') {
      free_pos[p].x = x;
      free_pos[p].y = y + 1;
      p++;
    }
    //South
    if(is_inside(x + 1, y) && world[x + 1][y].type == ' ') {
      free_pos[p].x = x + 1;
      free_pos[p].y = y;
      p++;
    }
    //West
    if(is_inside(x, y - 1) && world[x][y - 1].type == ' ') {
      free_pos[p].x = x;
      free_pos[p].y = y - 1;
      p++;
    }
  }
  
  if(p == 0){
    free_pos[0].x = x;
    free_pos[0].y = y;
    p = 1;
    if(current.num_gen == 0) {
      current.num_gen = 1;
    }
  }
  else {
    if(current.num_gen == 0) {
      // LOCK: Protege a célula atual ao criar filho raposa
      omp_set_lock(&cell_locks[x][y]);
      new = &new_world[x][y];
      new->type = current.type;
      new->num_gen = GEN_PROC_FOXES;
      new->num_food = GEN_FOOD_FOXES;
      omp_unset_lock(&cell_locks[x][y]);
      
      current.num_gen = GEN_PROC_FOXES + 1;
    }
  }
  
  new_pos_index = (x + y + current_gen) % p;
  new_pos = free_pos[new_pos_index];
  new = &new_world[new_pos.x][new_pos.y];
  
  // LOCK: Protege a célula de destino de conflitos (múltiplas raposas tentando mover para mesma célula)
  omp_set_lock(&cell_locks[new_pos.x][new_pos.y]);
  {
    if(new->type == 'F'){
      // Resolve conflito entre raposas
      if(current.num_gen - 1 < new->num_gen){
        new->num_gen = current.num_gen - 1;
        if(new->num_food != GEN_FOOD_FOXES)
          new->num_food = current.num_food - 1;
      }
      else if(current.num_gen - 1 == new->num_gen)
        if(current.num_food - 1 > new->num_food) {
          new->num_food = current.num_food - 1;
        }
    }
    else{
      if(new->type == 'R'){
        // Comeu um coelho
        new->num_food = GEN_FOOD_FOXES;
      }
      else{
        // Moveu para célula vazia
        new->num_food = current.num_food - 1;
      }
      new->num_gen = current.num_gen - 1;
      new->type = 'F';
    }
  }
  omp_unset_lock(&cell_locks[new_pos.x][new_pos.y]);
}

/* Processes movement and reproduction of all rabbits in the current generation */
void move_rabbits(){
  int x,y;
  
  // Copia raposas para new_world em paralelo (leitura é thread-safe)
  #pragma omp parallel for private(y) schedule(static)
  for(x = 0; x < R; x++) {
    for(y = 0; y < C; y++) {
      if(world[x][y].type == 'F') {
        new_world[x][y] = world[x][y];
      }
    }
  }
  
  // Move coelhos em paralelo com schedule dinâmico (melhor balanceamento)
  #pragma omp parallel for private(y) schedule(dynamic, 4)
  for(x = 0; x < R; x++) {
    for(y = 0; y < C; y++) {
      if(world[x][y].type == 'R') {
        move_rabbit(x, y);
      }
    }
  }
}

/* Processes movement, hunting, and reproduction of all foxes in the current generation */
void move_foxes(){
  int x,y;
  // Schedule dinâmico com chunk size de 4 linhas para melhor balanceamento
  #pragma omp parallel for private(y) schedule(dynamic, 4)
  for(x = 0; x < R; x++) {
    for(y = 0; y < C; y++) {
      if(world[x][y].type == 'F') {
        move_fox(x, y);
      }
    }
  }
}

/* Swaps the current world with the new world for the next generation */
void swap_worlds() {
  object **aux;
  aux = world;
  world = new_world;
  new_world = aux;
}

/* Outputs the final state of the world with all remaining objects and their positions */
void output() {
  int x,y;
  int n_objects = 0;
  
  // Conta objetos em paralelo
  #pragma omp parallel for private(y) reduction(+:n_objects) schedule(static)
  for(x = 0; x < R; x++) {
    for(y = 0; y < C; y++) {
      if(world[x][y].type != ' ')
        n_objects++;
    }
  }
  
  printf("%d %d %d %d %d %d %d\n",
         GEN_PROC_RABBITS,
         GEN_PROC_FOXES,
         GEN_FOOD_FOXES,
         0,
         R,
         C,
         n_objects);
         
  for(x = 0; x < R; x++) {
    for(y = 0; y < C; y++) {
      if(world[x][y].type == 'R')
        printf("RABBIT ");
      else if(world[x][y].type == 'F')
        printf("FOX ");
      else if(world[x][y].type == '*')
        printf("ROCK ");
      else
        continue;
      printf("%d %d\n", x, y);
    }
  }
}

/* Main function that initializes the ecosystem simulation and runs it for N_GEN generations */
int main(int argc, char *argv[]) {
  // Get number of threads from command line or environment
  if(argc > 1) {
    num_threads = atoi(argv[1]);
  } else {
    num_threads = omp_get_max_threads();
  }
  omp_set_num_threads(num_threads);
  
  scanf("%d", &GEN_PROC_RABBITS);
  scanf("%d", &GEN_PROC_FOXES);
  scanf("%d", &GEN_FOOD_FOXES);
  scanf("%d", &N_GEN);
  scanf("%d", &R);
  scanf("%d", &C);
  scanf("%d", &N);
  
  world = (object **)malloc(sizeof(object*) * R);
  new_world = (object **)malloc(sizeof(object*) * R);
  int i;
  for(i = 0; i < R; i++) {
    world[i] = (object *)malloc(C * sizeof(object));
    new_world[i] = (object *)malloc(C * sizeof(object));
  }
  
  // INICIALIZA LOCKS
  init_locks();
  
  init_world();
  fill_world();
  
  double start_time = omp_get_wtime();
  for(current_gen = 0; current_gen < N_GEN; current_gen++){
    move_rabbits();
    swap_worlds();
    reset_new_world();
    copy_rabbits();
    move_foxes();
    swap_worlds();
    reset_new_world();
  }
  double final_time = omp_get_wtime();
  
  printf("%.5lf\n",(final_time - start_time)*1000);
  output();
  
  // DESTRÓI LOCKS
  destroy_locks();
  
  // Free memory
  for(i = 0; i < R; i++) {
    free(world[i]);
    free(new_world[i]);
  }
  free(world);
  free(new_world);
  
  return 0;
}