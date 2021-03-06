#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include "huebar_color.h"

// START Kohonen Algorithm defines and global variables
#define MAP_WIDTH 80
#define MAP_HEIGHT 60
#define NORMALIZATION_VALUE 10000
#define pow2(x) ((x) * (x))
#define BIG_NUM          999999999999999999
#define LINE_SIZE        300
#define FALSE            0
#define TRUE             1
#define uint             unsigned int
#define uint64           unsigned long

typedef struct Neuron {
   unsigned int* components;
} Neuron;

typedef struct BMU {
   unsigned int x_coord;
   unsigned int y_coord;
} BMU;

typedef struct CentroidBMU {
   unsigned int x_coord;
   unsigned int y_coord;
   int count;
} CentroidBMU;

typedef struct Coordinate {
   float x;
   float y;
} Coordinate;

typedef struct Sample {
   unsigned int* components;
} Sample;

Neuron** map;
Sample* samples;
int total_components;
char** components_name;
uint* samples_max_components_values;
uint* samples_min_components_values;
int initial_radius = 80;
float round_radius;

// END Kohonen definitions and global variables

// START k-Means Algorithm defines and global variables
#define distance(i, j)   (datax(j) - datax(i)) * (datax(j) - datax(i)) + (datay(j) - datay(i)) * (datay(j) - datay(i))


typedef int bool;

int total_samples;

// END k-Means definitions and global variables

// START Kohonen algorithm methods

unsigned int randr(unsigned int min, unsigned int max)
{
  double scaled = (double)rand()/RAND_MAX;
  return (max - min +1)*scaled + min;
}

char** explode_string(char* a_str, const char a_delim, int* total_items)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);
    *total_items = count;

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        *(result + idx) = 0;
    }

    return result;
}

int stringToInteger(char a[]) {
  int c, sign, offset, n;

  if (a[0] == '-') {  // Handle negative integers
    sign = -1;
  }

  if (sign == -1) {  // Set starting position to convert
    offset = 1;
  }
  else {
    offset = 0;
  }

  n = 0;

  for (c = offset; (a[c] != '\0') && (a[c] != '\n'); c++) {
    n = n * 10 + a[c] - '0';
  }

  if (sign == -1) {
    n = -n;
  }

  return n;
}

void load_and_initialize_samples(char *filename)
{
  FILE *file;
  char line[LINE_SIZE];
  char** line_components;
  int ch, x, y, z, total_line_components;
  uint value;

  total_samples = 0;

  file = fopen(filename, "r");
  while(!feof(file)) {
    ch = fgetc(file);
    if(ch == '\n') {
      total_samples++;
    }
  }
  total_samples--;
  fclose(file);

  printf("\n\nTotal samples: %d\n\n", total_samples);

  // 'samples' is the data structure used to store the sample points for Kohonen algorithm process
  samples = (Sample *) malloc(sizeof(Sample) * total_samples);

  for(int i = 0; i < total_samples; i++) {
    samples[i].components = (unsigned int *) malloc(sizeof(unsigned int) * total_components);
  }

  // read data from file into array
  file = fopen(filename, "rt");
  fgets(line, LINE_SIZE, file);
  components_name = explode_string(line, ',', &total_components);

  samples_max_components_values = (uint*) malloc(sizeof(uint) * total_components);
  samples_min_components_values = (uint*) malloc(sizeof(uint) * total_components);

  // Initialize max values array to blank
  for(int e = 0; e < total_components; e++) {
    samples_max_components_values[e] = 0;
    samples_min_components_values[e] = 0;
  }

  // Load values from file
  for (int i = 0; i < total_samples; i++) {
    fgets(line, LINE_SIZE, file);
    line_components = explode_string(line, ',', &total_line_components);
    for(int e = 0; e < total_line_components; e++) {
      value = (unsigned int)(stringToInteger(line_components[e]));
      samples[i].components[e] = value;
      if(samples_max_components_values[e] < value) {
        samples_max_components_values[e] = value;
      }
      if(samples_min_components_values[e] > value) {
        samples_min_components_values[e] = value;
      }
    }
  }

  fclose(file);

  // Normalize values based on max value of each component from 0 to 255
  for (int i = 0; i < total_samples; i++) {
    for(int e = 0; e < total_components; e++) {
      value = samples[i].components[e];
      samples[i].components[e] = (unsigned int)(((value - samples_min_components_values[e]) * NORMALIZATION_VALUE)/(samples_max_components_values[e] - samples_min_components_values[e]));
    }
  }

}

void initialize_som_map()
{
  map = (Neuron **) malloc(sizeof(Neuron *) * MAP_WIDTH);

  int x, y;
  for(x = 0; x < MAP_WIDTH; x++) {
      map[x] = (Neuron *) malloc(sizeof(Neuron) * MAP_HEIGHT);
  }

  for(x = 0; x < MAP_WIDTH; x++) {
    for(y = 0; y < MAP_HEIGHT; y++) {
      map[x][y].components = (unsigned int *) malloc(sizeof(unsigned int) * total_components);

      for(int i=0; i<total_components;i++) {
        map[x][y].components[i] = randr(0,NORMALIZATION_VALUE);
      }
    }
  }

}

Sample* pick_random_sample() {
  int i = randr(0,total_samples - 1);
  return &samples[i];
}

Sample* pick_sample(int i) {
  return &samples[i];
}

uint distance_between_sample_and_neuron(Sample *sample, Neuron *neuron) {
  unsigned int euclidean_distance = 0;
  unsigned int component_diff;

  for(int i = 0; i < total_components; i++) {
    component_diff = sample->components[i] - neuron->components[i];
    euclidean_distance += pow2(component_diff);
  }

  return euclidean_distance;
  //return sqrt(euclidean_distance);
}

BMU* search_bmu(Sample *sample) {
  uint max_dist=999999999;
  uint dist = 0;
  BMU *bmu = (BMU *) malloc(sizeof(BMU));

  for(int x = 0; x < MAP_WIDTH; x++) {
    for(int y = 0; y < MAP_HEIGHT; y++) {
      dist = distance_between_sample_and_neuron(sample, &map[x][y]);
      if(dist < max_dist) {
        bmu->x_coord = x;
        bmu->y_coord = y;
        max_dist = dist;
      }
    }
  }
  return bmu;
}

float get_coordinate_distance(Coordinate *p1, Coordinate *p2) {
  float x_sub = (p1->x) - (p2->x);
  float y_sub = (p1->y) - (p2->y);
  return sqrt(x_sub*x_sub + y_sub*y_sub);
}

Coordinate* new_coordinate(float x, float y) {
  Coordinate *coordinate = malloc(sizeof(Coordinate));
  coordinate->x = x;
  coordinate->y = y;
  return coordinate;
}

void scale_neuron_at_position(int x, int y, Sample *sample, double scale) {

  float neuron_prescaled, neuron_scaled;
  Neuron *neuron = &map[x][y];

  for(int i=0; i<total_components; i++) {
    neuron_prescaled = neuron->components[i] * (1.0f-scale);
    neuron_scaled = (sample->components[i] * scale) + neuron_prescaled;
    neuron->components[i] = (int)neuron_scaled;
  }

}

void scale_neighbors(BMU *bmu, Sample *sample, float t) {

  float iteration_radius = roundf((float)(round_radius)*(1.0f-t));
  Coordinate *outer = new_coordinate(iteration_radius,iteration_radius);
  Coordinate *center = new_coordinate(0.0f,0.0f);
  float distance_normalized = get_coordinate_distance(center,outer);
  float distance;
  double scale;
  int x_coord;
  int y_coord;

  for(float y = -iteration_radius; y<iteration_radius; y++) {
    for(float x = -iteration_radius; x<iteration_radius; x++) {
      if((y + bmu->y_coord) >= 0 && (y + bmu->y_coord) < MAP_HEIGHT && (x + bmu->x_coord)>=0 && (x + bmu->x_coord) < MAP_WIDTH) {

        outer->x = x;
        outer->y = y;
        distance = get_coordinate_distance(outer,center);
        distance /= distance_normalized;

        scale = exp(-1.0f * (pow(distance, 2.0f)) / 0.15f);
        scale /= (t*4.0f + 1.0f); // +1 is to avoid divide by 0's

        x_coord = bmu->x_coord + x;
        y_coord = bmu->y_coord + y;
        scale_neuron_at_position(x_coord, y_coord, sample, scale);
      }
    }
  }

  free(outer);
  free(center);
}

void free_allocated_memory() {
  for(int e = 0; e < total_components; e++) {
    free(components_name[e]);
  }
  free(components_name);

  for(int i = 0; i < total_samples; i++) {
    free(samples[i].components);
  }
  free(samples);

  for(int x = 0; x < MAP_WIDTH; x++) {
    for(int y = 0; y < MAP_HEIGHT; y++) {
      free(map[x][y].components);
    }
    free(map[x]);
  }
  free(map);

  free(samples_max_components_values);
  free(samples_min_components_values);
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void output_html(BMU *final_bmus, bool auto_reload)
{
  bool found_bmu = FALSE;
  int diff_val, max_val, min_val, x_val, y_val, z_val, i, value, red, green, blue;
  uint x, y;
  float e;
  RGB *huebar = create_color_huebar(255);

  FILE *f = fopen("generated_kohonen_map.html", "w");
  if (f == NULL)
  {
      printf("Error opening file!\n");
      exit(1);
  }

  if(auto_reload) {
    fprintf(f, "<html><head><script>setTimeout(function(){ window.location.reload(1); }, 2500);</script></head><body>");
  } else {
    fprintf(f, "<html><head></head><body>");
  }

  fprintf(f, "<br/><h2>Components</h2>");


  for(int c = 0; c < total_components; c++) {

    fprintf(f, "<div style=\"width:500px;height:250px\">");
    fprintf(f, "<h3>%s</h3>", components_name[c]);
    fprintf(f, "<div style='position: absolute;'><table style='border-collapse: collapse;'>");
    // Search the min and max values of each vector component
    max_val = 0;
    min_val = 9999999;
    for(y = 0; y < MAP_HEIGHT; y++) {
      for(x = 0; x < MAP_WIDTH; x++) {
        x_val = (int)((map[x][y].components[c] * (samples_max_components_values[c] - samples_min_components_values[c]))/NORMALIZATION_VALUE);
        if(min_val > x_val) {
          min_val = x_val;
        }

        if(max_val < x_val) {
          max_val = x_val;
        }
      }
    }

    diff_val = max_val - min_val;

    for(y = 0; y < MAP_HEIGHT; y++) {
      fprintf(f, "<tr>");
      for(x = 0; x < MAP_WIDTH; x++) {
        value = (int)((map[x][y].components[c] * (samples_max_components_values[c] - samples_min_components_values[c]))/NORMALIZATION_VALUE);
        x_val = (int)(((value - min_val) * 255)/diff_val);
        RGB *color = &huebar[x_val];
        fprintf(f, "<td style='width:3px;height:3px;background-color:rgb(%d,%d,%d);' title='%d'></td>", color->r, color->g, color->b, value);
      }
      fprintf(f, "</tr>");
    }
    fprintf(f, "</table></div>");

    fprintf(f, "<div style='position: absolute; left: 420px;'><table style='border-collapse: collapse;'>");
    for(e = 0.0f; e < 255.0f; e+=2.86f) {
      RGB *color = &huebar[(int)e];
      if(e == 0.0f) {
        fprintf(f, "<tr><td style='width:3px;height:1px;background-color:rgb(%d,%d,%d);'><div style=\"position:absolute; width:50px;top:0px;text-align:left;\">&nbsp; %d</div><div style=\"position: absolute; text-align: left; width: 50px; top: 85px;\">&nbsp; %d</div><div style=\"position:absolute; width:50px;bottom:0px;text-align:left;\">&nbsp; %d</div></td></tr>", color->r, color->g, color->b, min_val, min_val + (max_val-min_val)/2, max_val);
      } else {
        fprintf(f, "<tr><td style='width:3px;height:1px;background-color:rgb(%d,%d,%d);'></td></tr>", color->r, color->g, color->b);
      }
    }
    fprintf(f, "</table></div>");

    fprintf(f, "</div>");
  }


  fprintf(f, "</tr></table>");

  free(huebar);
  fprintf(f, "</body></html>");
  fclose(f);
}

// END Kohonen algorithm methods

int main(int argc, char **argv)
{
    // usage: ./kohonen file_with_samples
    char *filename = argv[1];
    load_and_initialize_samples(filename);

    int MAX_TRAINING_ROUNDS = 10;
    float ROUND_INC = 1.0f/(float)(MAX_TRAINING_ROUNDS);
    float r = 0.0f;

    int MAX_ITER_PER_ROUND = total_samples * 30; // Number of iterations is 30 times the number of input samples
    float T_INC = 1.0f/(float)(MAX_ITER_PER_ROUND);
    float t = 0.0f;
    BMU *bmu;
    BMU *final_bmus;
    int iteration_num;
    int round_num;
    Sample *sample;

    // seed random
    srand(time(NULL));

    final_bmus = (BMU *)malloc(sizeof(BMU) * total_samples);

    initialize_som_map();
    //output_html(final_bmus, TRUE);

    round_radius = initial_radius;
    round_num = 0;

    while(r < 1.0f)
    {
       round_radius = (r == 0.0f) ? initial_radius : (round_radius/2.0f);
       printf("\nROUND %d/%d | INITIAL RADIUS: %d | RADIUS: %f\n", round_num, MAX_TRAINING_ROUNDS, initial_radius, round_radius);

       iteration_num = 0;
       t = 0.0f;

       while(t < 1.0f)
       {
         sample = pick_random_sample();
         bmu = search_bmu(sample); // Best Match Unit
         scale_neighbors(bmu, sample, t);
         free(bmu);

         t += T_INC;
         iteration_num++;

         //output_html(final_bmus, TRUE);
         //usleep(100000);
       }

       r += ROUND_INC;
       round_num++;
    }

    // Save the BMU coordinates in the SOM map
    for (int i = 0; i < total_samples; i++) {
      sample = pick_sample(i);
      bmu = search_bmu(sample); // Best Match Unit

      final_bmus[i].x_coord = (uint)bmu->x_coord;
      final_bmus[i].y_coord = (uint)bmu->y_coord;

      free(bmu);
    }

    output_html(final_bmus, FALSE);

    free(final_bmus);
    free_allocated_memory();

    return 0;
}
