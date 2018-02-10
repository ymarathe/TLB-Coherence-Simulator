//
//  TraceProcessor.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 2/7/18.
//  Copyright © 2018 Yashwant Marathe. All rights reserved.
//

#include "TraceProcessor.hpp"

void TraceProcessor::processPair(std::string name, std::string val)
{
    if (name.find("fmt") != std::string::npos)
    {
        if (val.find("m") != std::string::npos)
            strcpy(fmt, "-m");
        else
            strcpy(fmt, "-t");
    }
    
    if (name.find("cores") != std::string::npos)
        num_cores=(int)strtoul(val.c_str(), NULL, 10);
    
    for(int i = 0; i < num_cores; i++)
    {
        if (name == ("t" + std::to_string(i)))
            strcpy(trace[i], val.c_str());
        if (name == ("i" + std::to_string(i)))
            total_instructions_in_real_run[i] = strtoul(val.c_str(),NULL, 10);
        if (name == ("c" + std::to_string(i)))
            ideal_cycles_in_real_run[i] = strtoul(val.c_str(), NULL, 10);
        if (name == ("tl" + std::to_string(i)))
            num_tlb_misses_in_real_run[i] = strtoul(val.c_str(), NULL, 10);
        if (name == ("pw" + std::to_string(i)))
            avg_pw_cycles_in_real_run[i] = strtod(val.c_str(),NULL);
    }
    
    if (name == "l2d_lat")
        l2d_lat = strtoul(val.c_str(),NULL, 10);
    if (name == "l3d_lat")
        l3d_lat = strtoul(val.c_str(),NULL, 10);
    if (name == "vl_lat")
        vl_lat = strtoul(val.c_str(),NULL, 10);
    if (name == "dram_lat")
        dram_lat = strtoul(val.c_str(),NULL, 10);
    if (name == "vl_small_size")
        l3_small_tlb_size = 33554432;
    if (name == "vl_large_size")
        l3_large_tlb_size = 8388608 / 4;
}

void TraceProcessor::parseAndSetupInputs(char *input_cfg)
{
    std::ifstream file(input_cfg);
    std::string str;
    while (std::getline(file, str))
    {
        // Process str
        if ((str[0]=='/') && (str[1]=='/')) continue;
        
        // split str into arg name = arg value
        std::size_t found = str.find("=");
        if (found != std::string::npos)
        {
            std::string arg_name = str.substr(0,found);
            std::string arg_value = str.substr(found+1);
            found=arg_value.find("//");
            if (found != std::string::npos)
                arg_value = arg_value.substr(0,found);
            
            // strip leading or trailing whitespaces
            arg_name = trim(arg_name);
            arg_value = trim(arg_value);
            std::cout << " Found pair: " << arg_name << " and " << arg_value << std::endl;
            processPair(arg_name, arg_value);
        }
        else
            std::cout <<"Warning! Unknown entry: " << str << " found in cfg file. Exiting..."<< std::endl;
    }
    
    if (strcasecmp(fmt, "-m")!=0)
    {
        for (int i = 0;i < num_cores; i++)
        {
            total_instructions_in_real_run[i] = total_instructions_in_real_run[i]/num_cores;
            ideal_cycles_in_real_run[i] = ideal_cycles_in_real_run[i]/num_cores;
            num_tlb_misses_in_real_run[i] = num_tlb_misses_in_real_run[i]/num_cores;
            avg_pw_cycles_in_real_run[i] = avg_pw_cycles_in_real_run[i];
        }
    }
}

void TraceProcessor::verifyOpenTraceFiles()
{
    for (int i = 0 ;i < num_cores; i++)
    {
        trace_fp[i] = fopen((char*)trace[i], "r");
        if (!trace_fp[i])
        {
            std::cout << "[Error] Check trace file path of core " << i << std::endl;
            std::cout << trace[i] << " does not exist" << std::endl;
            exit(0);
        }
    }
}

int TraceProcessor::getNextEntry()
{
    int index = -1;
    uint64_t least = 0xffffffffffffffff;
    
    if(!strcasecmp(fmt,"-m"))
    {
        for(int i = 0; i < num_cores; i++)
        {
            if(used_up[i])
            {
                // if not end-of-file, fill next entry
                if(!empty_file[i])
                {
                    if (!feof(trace_fp[i]))
                    {
                        if (strcasecmp(fmt, "-m") == 0)
                        {
                            fread ((void*)&buf1[i], sizeof(trace_tlb_entry_t), 1, trace_fp[i] );
                        }
                        else
                        {
                            fread ((void*)&buf2[i], sizeof(trace_tlb_tid_entry_t), 1, trace_fp[i] );
                        }
                        used_up[i] = false;
                        entry_count[i]++;
                    }
                    else
                    {
                        std::cout << " Done with core " << i << std::endl;
                        empty_file[i] = true;
                    }
                }
            }
            if (!empty_file[i])
            {
                if (strcasecmp(fmt, "-m") == 0)
                {
                    if (buf1[i].ts < least)
                    {
                        least = buf1[i].ts;
                        index = i;
                    }
                }
                else
                {
                    if (buf2[i].ts < least)
                    {
                        least = buf2[i].ts;
                        index = i;
                    }
                }
            }
        }
    }
    else
    {
        // there's just one file.
        if(getc(trace_fp[0]) == EOF)
            index = -1;
        else
        {
            fread ((void*)&buf2[0], sizeof(trace_tlb_tid_entry_t), 1, trace_fp[0] );
            global_index = (++global_index) % num_cores;
            index = global_index;
            
            std::cout << "is_large::" << buf2[0].large << std::endl;
        }
    }
    
    return index;
}

Request TraceProcessor::generateRequest()
{
    int idx = getNextEntry();
    uint64_t va, tid;
    bool is_large, is_write;
    bool is_multicore = strcasecmp(fmt, "-m") == 0;
    
    if(idx != -1)
    {
        if (is_multicore)
        {
            va = buf1[idx].va;
            is_large = buf1[idx].large;
            is_write = (bool)((buf1[idx].write != 0)? true: false);
            last_ts[idx] = buf1[idx].ts;
        }
        else
        {
            va = buf2[0].va;
            is_large = buf2[0].large;
            is_write = (bool)((buf2[0].write!=0)? true: false);
            tid = buf2[0].tid;
            last_ts[idx] = buf2[0].ts;
        }
        
        if(curr_ts[idx] >= last_ts[idx])
        {
            //TODO: ymarathe:: thread id is the same as core id right now.
            //Hyperthreading?
            Request req(va, is_write ? DATA_WRITE : DATA_READ, idx, is_large, idx);
            last_ts[idx]++;
            return req;
        }
        else
        {
            last_ts[idx]++;
        }
    }
    
    return Request();
}
