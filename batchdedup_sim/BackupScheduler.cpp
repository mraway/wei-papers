#include "BackupScheduler.h"


void BackupScheduler::setMachineList(vector<vector<double> > machine_loads) {
    machines=machine_loads;
}


double AllScheduler::schedule_round(std::vector<std::vector<double> > &round_schedule) {
}
const char * AllScheduler::getName() {
    return "All Scheduler";
}
