import os
import re
import sniper_lib

root_dir = "../results"

bench_list = os.listdir(root_dir)
for bench in bench_list:
    total_i = 0
    total_c = 0
    region_list = os.listdir(os.path.join(root_dir, bench))
    region_count = len(region_list)
    for region in region_list:
        # process parameters of the region
        match = re.match("(.+)\\.(.+)_(\\d+)_t(\\d+)r(\\d+)_warmup(\\d+)_prolog(\\d+)_region(\\d+)_epilog(\\d+)_(\\d+)_(\\d+)-(\\d+).(\\d+)", region)
        program_name = match[1]
        run_name = match[2]
        run_number = int(match[3])
        thread_number = int(match[4])
        region_number = int(match[5])
        weight = int(match[12])
        # process results of the simulation
        dir = os.path.join(root_dir, bench, region)
        results = sniper_lib.get_results(resultsdir=dir)['results']
        i = results['performance_model.instruction_count'][0]
        c = results['performance_model.cycle_count'][0]
        total_i += i * weight
        total_c += c * weight
    total_ipc = total_i / total_c
    total_cpi = total_c / total_i
    print(program_name, total_ipc, total_cpi)
