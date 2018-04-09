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

        is_multicore = (strcasecmp(fmt, "-m") == 0);
    }
    
    if (name.find("cores") != std::string::npos)
        num_cores=(int)strtoul(val.c_str(), NULL, 10);
    
    for(int i = 0; i < num_cores; i++)
    {
        if (name == ("t" + std::to_string(i)))
            strcpy(trace[i], val.c_str());
        if (name == ("shootdown"))
            strcpy(shootdown, val.c_str());
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
    
    if (strcasecmp(fmt, "-m") != 0)
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
    for (int i = 0 ; (i < num_cores) && is_multicore; i++)
    {
        trace_fp[i] = fopen((char*)trace[i], "r");
        if (!trace_fp[i])
        {
            std::cout << "[Error] Check trace file path of core " << i << std::endl;
            std::cout << trace[i] << " does not exist" << std::endl;
            exit(0);
        }
        fread ((void*)&buf1[i], sizeof(trace_tlb_entry_t), 1, trace_fp[i]);
        used_up[i] = false;
        empty_file[i] = false;
        entry_count[i] = 1;
    }

    if(!is_multicore)
    {
        trace_fp[0] = fopen((char*)trace[0], "r");
        if (!trace_fp[0])
        {
            std::cout << "[Error] Check trace file path" << std::endl;
            std::cout << trace[0] << " does not exist" << std::endl;
            exit(0);
        }
        fread ((void*)&buf2[0], sizeof(trace_tlb_tid_entry_t), 1, trace_fp[0]);
        used_up[0] = false;
        empty_file[0] = false;
        entry_count[0] = 1;
    }

    shootdown_fp = fopen((char*) shootdown, "r");
    if(!shootdown_fp)
    {
        std::cout << "[Error] Check shootdown file path" << std::endl;
        std::cout << shootdown << " does not exist" << std::endl;
        exit(0);
    }

    fread((void*)buf3, sizeof(trace_shootdown_entry_t), 1, shootdown_fp);
    used_up_shootdown = false;

}

void TraceProcessor::getShootdownEntry()
{
    if(used_up_shootdown)
    {
        if(!empty_file_shootdown && !feof(shootdown_fp))
        {
            fread((void*)buf3, sizeof(trace_shootdown_entry_t), 1, shootdown_fp);
            std::cout << "[TLB_SHOOTDOWN_NEXT] = " << buf3->ts << "," << buf3->core_id << "," << buf3->num_cores << "\n";
            used_up_shootdown = false;
        }
        else
        {
            std::cout << "Done with shootdown file\n";
            empty_file_shootdown = true;
        }
    }
}

int TraceProcessor::getNextEntry()
{
    int index = -1;
    uint64_t least = 0xffffffffffffffff;
    
    for(int i = 0; (i < num_cores) && is_multicore; i++)
    {
        if(used_up[i])
        {
            // if not end-of-file, fill next entry
            if(!empty_file[i])
            {
                if (!feof(trace_fp[i]))
                {
                    fread ((void*)&buf1[i], sizeof(trace_tlb_entry_t), 1, trace_fp[i] );
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
            if (buf1[i].ts < least)
            {
                least = buf1[i].ts;
                index = i;
            }
        }
    }

    if(!is_multicore)
    {
        index = 0;

        if(used_up[0])
        {
            if(!empty_file[0])
            {
                if(!feof(trace_fp[0]))
                {
                    fread((void*)&buf2[0], sizeof(trace_tlb_tid_entry_t), 1, trace_fp[0]);
                    used_up[0] = false;
                    entry_count[0]++;
                }
                else
                {
                    std::cout << "Done with trace " << "\n";
                    empty_file[0] = true;
                    index = -1;
                }
            }
        }
    }
    
    return index;
}

Request* TraceProcessor::generateRequest()
{
    int idx = getNextEntry();
    getShootdownEntry();

    uint64_t shootdown_va;
    uint64_t va, tid;
    bool is_large, is_write;
    bool is_multicore = strcasecmp(fmt, "-m") == 0;

    uint64_t shootdown_ts = buf3->ts;
    int shootdown_core_id = buf3->core_id;
    int shootdown_num_cores = buf3->num_cores;

    if(idx != -1)
    {
        if (is_multicore)
        {
            va = buf1[idx].va;
            is_large = buf1[idx].large;
            is_write = (bool)((buf1[idx].write != 0)? true: false);
            curr_ts[idx] = buf1[idx].ts;

            if(curr_ts[idx] == last_ts[idx])
            {
                Request *req = new Request(va, is_write ? DATA_WRITE : DATA_READ, idx, is_large, idx);
                used_up[idx] = true;

                //add_to_presence_map(*req);

                return req;
            }

            if((shootdown_ts <= (last_ts[idx])) && (shootdown_core_id == idx))
            {

                std::cout << "Shootdown timestamp = " << shootdown_ts << std::endl;

                //Randomly choose either large page or small page
                Request *req = nullptr;
                int num_tries = 0;

                while(req == nullptr)
                {
                    //double randVal = rand();
                    //auto chosen_map = (randVal < 0.5) ? presence_map_small_page : presence_map_large_page;
                    //bool shootdown_is_large = (randVal < 0.5) ? false : true;
                    auto chosen_map = (num_tries % 2 == 0) ? presence_map_small_page : presence_map_large_page;
                    bool shootdown_is_large = (num_tries % 2 == 0) ? false : true;

                    if((num_tries % 2 == 0) && (num_tries > 0))
                    {
                        std::cout << "[CHANGE_SHOOTDOWN_CORES] Could not find adequate address, changing number of cores affected\n";
                        shootdown_num_cores = (shootdown_num_cores != (NUM_CORES - 1)) ? (shootdown_num_cores + 1) % NUM_CORES : 
                                                   (num_tries == (2 * NUM_CORES - 1)) ? 1 : 2;
                    }

                    for(auto it = chosen_map.begin(); it != chosen_map.end(); it++)
                    {
                        //if(it->second.size() == (shootdown_num_cores - 1))
                        //if(it->second.size() == (shootdown_num_cores) && (it->second.find(shootdown_core_id) != it->second.end()))
                        if(((num_tries < NUM_CORES * 2) && it->second.size() == (shootdown_num_cores) && (it->second.find(shootdown_core_id) != it->second.end())) || \
                                ((num_tries == NUM_CORES * 2) && (it->second.find(shootdown_core_id) != it->second.end())))
                        {
                            shootdown_va = it->first.m_addr;
                            req = new Request(shootdown_va, TRANSLATION_WRITE, idx, shootdown_is_large, shootdown_core_id);
                            used_up_shootdown = true;
                            goto exit_loop_mc;
                        }
                    }

                    num_tries += 1;

                }
                exit_loop_mc:
                std::cout << "[TLB_SHOOTDOWN_REQ]: Generating " << std::hex << (*req) << std::dec;
                std::cout << "Num cores affected = " << shootdown_num_cores << "\n";
                
                RequestDesc rdesc(req->m_addr, req->m_tid, req->m_is_large);
                std::cout << rdesc << ": ";
                if(req->m_is_large)
                {
                    for(auto it = presence_map_large_page[rdesc].begin(); it != presence_map_large_page[rdesc].end(); it++)
                    {
                        std::cout << *it << ", ";
                    }
                    std::cout << "\n";
                }
                else
                {
                    for(auto it = presence_map_small_page[rdesc].begin(); it != presence_map_small_page[rdesc].end(); it++)
                    {
                        std::cout << *it << ", ";
                    }
                    std::cout << "\n";
                }

                return req;
            }
            else if(curr_ts[idx] > last_ts[idx])
            {
                Request *req = new Request();
                req->m_is_memory_acc = false;
                req->m_core_id = idx;
                last_ts[idx]++;

                if(last_ts[idx] % 1000000 == 0)
                {
                    std::cout << "[NUM_INSTR_PROCESSED] Core : " << idx << ", Count = " << last_ts[idx] << "\n";
                }
                return req;
            }
        }
        else
        {
            va = buf2[idx].va;
            is_large = buf2[idx].large;
            is_write = (bool)((buf2[idx].write != 0)? true: false);
            curr_ts[idx] = buf2[idx].ts;
            tid = buf2[idx].tid;
            unsigned int core = (tid + tid_offset) % NUM_CORES;

            if(curr_ts[idx] == global_ts)
            {
                //Threads switch about every context switch interval
                //uint64_t tid = (idx + tid_offset) % NUM_CORES;
                Request *req = new Request(va, is_write ? DATA_WRITE : DATA_READ, tid, is_large, core);
                used_up[idx] = true;

                last_ts[core] = global_ts;

                //add_to_presence_map(*req);

                //For every instruction added, decrement context_switch_count
                context_switch_count--;
                if(context_switch_count == 0)
                {
                    tid_offset = switch_threads();
                }
                return req;
            }

            if(shootdown_ts <= global_ts)
            {
                //Randomly choose either large page or small page
                Request *req = nullptr;
                int num_tries = 0;

                while(req == nullptr)
                {
                    //double randVal = rand();
                    //auto chosen_map = (randVal < 0.5) ? presence_map_small_page : presence_map_large_page;
                    //bool shootdown_is_large = (randVal < 0.5) ? false : true;
                    auto chosen_map = (num_tries % 2 == 0) ? presence_map_small_page : presence_map_large_page;
                    bool shootdown_is_large = (num_tries % 2 == 0) ? false : true;

                    if((num_tries % 2 == 0) && (num_tries > 0))
                    {
                        std::cout << "[CHANGE_SHOOTDOWN_CORES] Could not find adequate address, changing number of cores affected\n";
                        shootdown_num_cores = (shootdown_num_cores != (NUM_CORES - 1)) ? (shootdown_num_cores + 1) % NUM_CORES : 
                                                   (num_tries == (2 * NUM_CORES - 1)) ? 1 : 2;
                    }

                    for(auto it = chosen_map.begin(); it != chosen_map.end(); it++)
                    {
                        //If the translation entry is present in the initiator core
                        //And if the number of cores having the translation entries = Number of victim cores
                        if(((num_tries < NUM_CORES * 2) && it->second.size() == (shootdown_num_cores) && (it->second.find(shootdown_core_id) != it->second.end())) || \
                                ((num_tries == NUM_CORES * 2) && (it->second.find(shootdown_core_id) != it->second.end())))
                        {
                            shootdown_va = it->first.m_addr;
                            req = new Request(shootdown_va, TRANSLATION_WRITE, tid, shootdown_is_large, shootdown_core_id);
                            used_up_shootdown = true;
                            goto exit_loop_mt;
                        }
                    }

                    num_tries += 1;
                    if(num_tries > NUM_CORES * 2)
                    {
                        req = nullptr;
                        used_up_shootdown = true;
                        std::cout << "[GIVING_UP]\n";
                        goto exit_loop_mt;
                    }
                }
                exit_loop_mt:
                if(req != nullptr)
                {
                    std::cout << "[TLB_SHOOTDOWN_REQ]: Generating " << std::hex << (*req) << std::dec;
                    std::cout << "Num cores affected = " << shootdown_num_cores << "\n";

                    RequestDesc rdesc(req->m_addr, req->m_tid, req->m_is_large);
                    std::cout << rdesc << ": ";
                    if(req->m_is_large)
                    {
                        for(auto it = presence_map_large_page[rdesc].begin(); it != presence_map_large_page[rdesc].end(); it++)
                        {
                            std::cout << *it << ", ";
                        }
                        std::cout << "\n";
                    }
                    else
                    {
                        for(auto it = presence_map_small_page[rdesc].begin(); it != presence_map_small_page[rdesc].end(); it++)
                        {
                            std::cout << *it << ", ";
                        }
                        std::cout << "\n";
                    }
                }

                return req;
            }
            else if(curr_ts[idx] > global_ts)
            {
                Request *req = new Request();
                req->m_is_memory_acc = false;
                req->m_core_id = (global_ts) % NUM_CORES;
                global_ts++;

                //For every instruction added, decrement context_switch_count
                context_switch_count--;
                if(context_switch_count == 0)
                {
                    tid_offset = switch_threads();
                }

                if(global_ts % 1000000 == 0)
                {
                    std::cout << "[NUM_INSTR_PROCESSED] : Count = " << global_ts << "\n";
                }
                return req;
            }

            if(context_switch_count % 100000 == 0)
            {
                std::cout << "Context switch count = " << context_switch_count << "\n";
            }

        }

    }
    
    return nullptr;
}

uint64_t TraceProcessor::switch_threads()
{
    //When context switch count is 0, reinitialize tid offset
    context_switch_count = (5000000000 - 3000000000) * (rand()/(double) RAND_MAX);
    uint64_t tid_offset = (NUM_CORES) * (rand()/(double) RAND_MAX);
    std::cout << "Switching threads\n";

    for(int i = 0; i < NUM_CORES; i++)
    {
        uint64_t tid = (i + tid_offset) % NUM_CORES;
        std::cout << "Core " << i << " now running thread = " << tid << "\n";
    }

    return tid_offset;
}

void TraceProcessor::add_to_presence_map(Request &r)
{
    RequestDesc rdesc(r.m_addr, r.m_tid, r.m_is_large);

    if(rdesc.m_is_large)
    {
        rdesc.m_addr = (rdesc.m_addr) & ~((1 << 21) - 1);
        if(presence_map_large_page.find(rdesc) != presence_map_large_page.end())
        {
            presence_map_large_page[rdesc].insert(r.m_core_id);
        }
        else
        {
            //presence_map_large_page[rdesc] = std::set<uint64_t>();
            presence_map_large_page.insert(std::pair<RequestDesc, std::set<uint64_t>>(rdesc, std::set<uint64_t>()));
            presence_map_large_page[rdesc].insert(r.m_core_id);
        }
    }
    else
    {
        rdesc.m_addr = (rdesc.m_addr) & ~((1 << 12) - 1);
        if(presence_map_small_page.find(rdesc) != presence_map_small_page.end())
        {
            presence_map_small_page[rdesc].insert(r.m_core_id);
        }
        else
        {
            //presence_map_small_page[rdesc] = std::set<uint64_t>();
            presence_map_small_page.insert(std::pair<RequestDesc, std::set<uint64_t>>(rdesc, std::set<uint64_t>()));
            presence_map_small_page[rdesc].insert(r.m_core_id);
        }
    }
}

void TraceProcessor::remove_from_presence_map(uint64_t addr, uint64_t tid, bool is_large, unsigned int core_id)
{
    RequestDesc rdesc(addr, tid, is_large);

    if(is_large)
    {
        //assert(presence_map_large_page.find(rdesc) != presence_map_large_page.end());
        if(presence_map_large_page.find(rdesc) != presence_map_large_page.end())
        {
            auto it = presence_map_large_page[rdesc].find(core_id);
            //assert(it != presence_map_large_page[rdesc].end());
            if(it != presence_map_large_page[rdesc].end())
            {
                presence_map_large_page[rdesc].erase(it);
                if(presence_map_large_page[rdesc].size() == 0)
                {
                    presence_map_large_page.erase(rdesc);
                }
            }
        }
    }
    else
    {
        //assert(presence_map_small_page.find(rdesc) != presence_map_small_page.end());
        if(presence_map_small_page.find(rdesc) != presence_map_small_page.end())
        {
            auto it = presence_map_small_page[rdesc].find(core_id);
            //assert(it != presence_map_small_page[rdesc].end());
            if(it != presence_map_small_page[rdesc].end())
            {
                presence_map_small_page[rdesc].erase(it);
                if(presence_map_small_page[rdesc].size() == 0)
                {
                    presence_map_small_page.erase(rdesc);
                }
            }
        }
    }
}
