typedef struct {
  int bold;
  int underline;
  int foreground;
  int background;
  int reverse;
  int caps;
  int repeat;
  int advertise;
  int url;
} ss_points;

typedef struct {
  int id;
  int threshold;
  int decrease;
  int warn;
  int fwarn;
  int kick;
  int ban;
} ss_profile;

extern ss_profile  ssdefaultprofile;
extern ss_points sspoints;

float spamscan_loop_line(char *line);
int spamscan_check_repeat(const char *new, const char *old);
float spamscan_calc_controlcodes(int points, int threshold, int count, int total);
void ss_decrease(void);
