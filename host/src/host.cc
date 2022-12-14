// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <openenclave/host.h>
#include <stdio.h>
#include <iostream>
#include "zmq_comm.hpp"
#include "absl/strings/str_split.h"

// Include the untrusted helloworld header that is generated
// during the build. This file is generated by calling the
// sdk tool oeedger8r against the helloworld.edl file.
#include "../helloworld_u.h"

enum mode_type
{
    RUN_BOTH_CLIENT_AND_SERVER,
    RUN_CLIENT_ONLY,
    LISTENER_MODE,
    COORDINATOR_MODE,
    JS_MODE,
    USER_MODE,
    WORKER_MODE,
    SYNC_SERVER_MODE,
    CAPSULEDB_MODE
};

#define PORT_NUM 1234

// Flags for PSL
DEFINE_int32(mode, -1, "Configures which mode to run KVS in");

DEFINE_string(hosts, "", "Comma seperated list of IP addresses");

DEFINE_string(server_address, "", "Address of the KVS coordinator");
DEFINE_int32(port, 0, "Port that the server listens to");

DEFINE_string(scenario, "", "Path to enclave to load");
DEFINE_string(algorithm, "", "Path to enclave to load");
DEFINE_string(coordinator, "", "Path to enclave to load");

DEFINE_string(jobs, "4", "Path to enclave to load");
DEFINE_string(env, "", "Path to enclave to load");
DEFINE_string(env_frame, "", "Path to enclave to load");

DEFINE_string(robot, "", "Path to enclave to load");
DEFINE_string(goal, "", "Path to enclave to load");
DEFINE_string(goal_radius, "", "Path to enclave to load");

DEFINE_string(start, "", "Path to enclave to load");
DEFINE_string(min, "", "Path to enclave to load");
DEFINE_string(max, "", "Path to enclave to load");

DEFINE_string(problem_id, "", "Path to enclave to load");
DEFINE_string(time_limit, "", "Path to enclave to load");
DEFINE_string(check_resolution, "", "Path to enclave to load");

DEFINE_string(discretization, "", "Path to enclave to load");
DEFINE_string(is_float, "", "Path to enclave to load");

DEFINE_string(input_file, "", "JS input file to execute!");

bool check_simulate_opt(int* argc, char* argv[])
{
    for (int i = 0; i < *argc; i++)
    {
        if (strcmp(argv[i], "--simulate") == 0)
        {
            fprintf(stdout, "Running in simulation mode\n");
            memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(char*));
            (*argc)--;
            return true;
        }
    }
    return false;
}

void thread_run_zmq_client(unsigned thread_id, Asylo_SGX* sgx){
    LOG(INFO) << "[thread_run_zmq_client_worker]";
    zmq_comm zs = zmq_comm(NET_WORKER_IP, thread_id, nullptr);
    zs.run_client();
}

void thread_run_zmq_js_client(unsigned thread_id, Asylo_SGX* sgx){
    LOG(INFO) << "[thread_run_zmq_client_worker]";
    zmq_comm zs = zmq_comm(NET_WORKER_IP, thread_id, nullptr);
    zs.run_js_client();
}

void thread_run_zmq_router(unsigned thread_id){
    LOG(INFO) << "[thread_run_zmq_server]"; 
    zmq_comm zs = zmq_comm(NET_SEED_ROUTER_IP, thread_id, nullptr);
    zs.run_server();
}

void thread_start_fake_client(Asylo_SGX* sgx){
    sgx->execute();
}

void thread_start_coordinator(Asylo_SGX* sgx){
    sgx->execute_coordinator();
}

void thread_crypt_actor_thread(Asylo_SGX* sgx){
    sgx->start_crypt_actor_thread();
}

zmq::message_t string_to_message(const std::string& s) {
    zmq::message_t msg(s.size());
    memcpy(msg.data(), s.c_str(), s.size());
    return msg;
}

std::string message_to_string(const zmq::message_t& message) {
    return std::string(static_cast<const char*>(message.data()), message.size());
}

unsigned long int get_current_time(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void thread_user_receiving_result(){
    zmq::context_t context (1);
    zmq::socket_t socket_result(context, ZMQ_PULL);
    socket_result.bind ("tcp://*:" + std::to_string( NET_USER_RECV_RESULT_PORT));
    std::vector<zmq::pollitem_t> pollitems = {
            { static_cast<void *>(socket_result), 0, ZMQ_POLLIN, 0 },
    };
    while (true) {
        // LOG(INFO) << "Start zmq";
        zmq::poll(pollitems.data(), pollitems.size(), 0);
        // Join Request
        if (pollitems[0].revents & ZMQ_POLLIN) {
            zmq::message_t message;
            socket_result.recv(&message);
            std::string result = message_to_string(message);
            std::vector<std::string> split =absl::StrSplit(result, "@@@");
            std::cout << "> "  << split[3] << std::endl;
        }
    }
}

int run_clients_only(){
    // std::unique_ptr <asylo::SigningKey> signing_key = asylo::EcdsaP256Sha256SigningKey::Create().ValueOrDie();
    // asylo::CleansingVector<uint8_t> serialized_signing_key;
    // ASSIGN_OR_RETURN(serialized_signing_key,
    //                         signing_key->SerializeToDer());

    std::vector <std::thread> worker_threads;
    unsigned long int now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    //start clients
    int num_threads = TOTAL_THREADS + 1;
    for (unsigned thread_id = START_CLIENT_ID; thread_id < num_threads; thread_id++) {
        Asylo_SGX* sgx = new Asylo_SGX( std::to_string(thread_id));
        sgx->init();
        sgx->setTimeStamp(now);
        sleep(1);
        worker_threads.push_back(std::thread(thread_run_zmq_client, thread_id, sgx));
        worker_threads.push_back(std::thread(thread_start_fake_client, sgx));
    }
    sleep(1 * 1000 * 1000);
    return 0; 
}

int run_client_and_router() {

    // std::unique_ptr <asylo::SigningKey> signing_key = asylo::EcdsaP256Sha256SigningKey::Create().ValueOrDie();
    // asylo::CleansingVector<uint8_t> serialized_signing_key;
    // ASSIGN_OR_RETURN(serialized_signing_key,
    //                         signing_key->SerializeToDer());

    // thread assignments:
    // thread 0: multicast router
    // thread 1: coordinator
    // thread 2-n: clients
    std::vector <std::thread> worker_threads;
    //start clients
    unsigned long int now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    LOG(INFO) << (now);
    for (unsigned thread_id = START_CLIENT_ID; thread_id < TOTAL_THREADS; thread_id++) {
        Asylo_SGX* sgx = new Asylo_SGX( std::to_string(thread_id));
        sgx->init();
        sgx->setTimeStamp(now);
        sleep(1);
        if(thread_id == 1){
            worker_threads.push_back(std::thread(thread_run_zmq_client, thread_id, sgx));
            worker_threads.push_back(std::thread(thread_start_coordinator, sgx));
        } else{
            worker_threads.push_back(std::thread(thread_run_zmq_client, thread_id, sgx));
            worker_threads.push_back(std::thread(thread_start_fake_client, sgx));
        }

    }
    sleep(2);

    //start router
    worker_threads.push_back(std::thread(thread_run_zmq_router, 0));
    sleep(1 * 1000 * 1000);
    return 0;
}

int run_js() {
    // std::unique_ptr <asylo::SigningKey> signing_key(std::move(asylo::EcdsaP256Sha256SigningKey::CreateFromPem(
    //                                         signing_key_pem)).ValueOrDie());

    // asylo::CleansingVector<uint8_t> serialized_signing_key;
    // ASSIGN_OR_RETURN(serialized_signing_key,
    //                         signing_key->SerializeToDer());

    std::vector <std::thread> worker_threads;
    Asylo_SGX* sgx = new Asylo_SGX("1");

    sgx->init();

    sleep(1);
    sgx->execute();
    std::string s = FLAGS_input_file;
    sgx->execute_js_file(s);
    LOGI << "finished running the code";
    return 0; 
}

int run_local_dispatcher(){

    zmq::context_t context (1);
    zmq::socket_t socket_from_user(context, ZMQ_PULL);
    socket_from_user.bind ("tcp://*:" +  std::to_string(NET_COORDINATOR_FROM_USER_PORT));

    zmq::socket_t socket_for_membership(context, ZMQ_PULL);
    socket_for_membership.bind ("tcp://*:" +  std::to_string(NET_COORDINATOR_RECV_MEMBERSHIP_PORT));

    zmq::socket_t socket_for_result(context, ZMQ_PULL);
    socket_for_result.bind ("tcp://*:"+ std::to_string(NET_COORDINATOR_RECV_RESULT_PORT));


    // poll for new messages
    std::vector<zmq::pollitem_t> pollitems = {
            { static_cast<void *>(socket_from_user), 0, ZMQ_POLLIN, 0 },
            { static_cast<void *>(socket_for_membership), 0, ZMQ_POLLIN, 0 },
            { static_cast<void *>(socket_for_result), 0, ZMQ_POLLIN, 0 },
    };

    std::string code = "";
    std::string return_addr = "localhost";
    while (true) {
        // LOG(INFO) << "Start zmq";
        zmq::poll(pollitems.data(), pollitems.size(), 0);
        // Join Request
        // receive code from user
        //
        if (pollitems[0].revents & ZMQ_POLLIN) {
            //Get code from client
            zmq::message_t message;
            socket_from_user.recv(&message);
            //code = message_to_string(message);
            LOGI << message_to_string(message);
            std::vector<std::string> splitted_messages = absl::StrSplit(message_to_string(message), GROUP_ADDR_DELIMIT, absl::SkipEmpty());
            return_addr = splitted_messages[0];
            code = splitted_messages[1];
            LOG(INFO) << "[Client " << return_addr << "]:  " + code ;

            //aloha to query for available worker nodes
            zmq::socket_t* socket_ptr  = new  zmq::socket_t( context, ZMQ_PUSH);
            socket_ptr -> connect ("tcp://" + std::string(NET_SEED_ROUTER_IP) + ":" + std::to_string(NET_SERVER_CONTROL_PORT));
            socket_ptr -> send(string_to_message("tcp://" + std::string(NET_JS_TASK_COORDINATOR_IP) +":"));
        }

        //receive
        if (pollitems[1].revents & ZMQ_POLLIN) {
            zmq::message_t message;
            socket_for_membership.recv(&message);
            LOG(INFO)  <<message_to_string(message);
            std::vector<std::string> addresses = absl::StrSplit(message_to_string(message), GROUP_ADDR_DELIMIT, absl::SkipEmpty());
            zmq::socket_t* socket_to_worker;
            int thread_count = 2;
            for( const std::string& a : addresses ) {

                std::vector<std::string> address = absl::StrSplit(a, ":", absl::SkipEmpty());
                LOG(INFO)  << address[2];
                int port = std::stoi(address[2]);
                std::string worker_addr = "tcp:" + address[1] + ":" + std::to_string(port + (NET_WORKER_LISTEN_FOR_TASK_BASE_PORT - NET_CLIENT_BASE_PORT));
                LOG(INFO)  << "[RECEIVED WORKER ADDR] " + worker_addr ;
                socket_to_worker  = new  zmq::socket_t( context, ZMQ_PUSH);
                //"tcp://" + std::string(NET_WORKER_IP) + ":" + std::to_string(NET_WORKER_LISTEN_FOR_TASK_BASE_PORT +  thread_count)
                socket_to_worker -> connect (worker_addr);
                //LOGI << "[connected to recv_code_port] " << std::to_string(NET_WORKER_LISTEN_FOR_TASK_BASE_PORT +  thread_count);
                thread_count += 1;
                socket_to_worker->send(string_to_message(code));
            }
        }

        if (pollitems[2].revents & ZMQ_POLLIN) {
            zmq::message_t message;
            socket_for_result.recv(&message);
            std::string result = message_to_string(message);
            LOGI << "return sent to " << return_addr << " " <<  result;

            zmq::socket_t* socket_ptr  = new  zmq::socket_t( context, ZMQ_PUSH);
            socket_ptr -> connect (return_addr);
            socket_ptr->send(string_to_message(result));
        }

    }

    return 0;
}

int run_user(){
    zmq::context_t context (1);
    // socket for join requests
    std::vector <std::thread> worker_threads;
    worker_threads.push_back(std::thread(thread_user_receiving_result));

    zmq::socket_t* socket_send  = new  zmq::socket_t( context, ZMQ_PUSH);
    socket_send -> connect ("tcp://" + std::string(NET_JS_TASK_COORDINATOR_IP) + ":" + std::to_string(NET_COORDINATOR_FROM_USER_PORT));
//    std::ifstream t("/opt/my-project/src/input.js");
//    std::stringstream buffer;
//    buffer << t.rdbuf();
//    std::string code = buffer.str();
//    socket_send->send(string_to_message(code));
    std::string cmd;
    std::string cmd_buffer = "";
    bool first_cmd_wait = false;
    unsigned long int now =get_current_time();
    while(std::getline(std::cin, cmd)){
        cmd_buffer += cmd;
        cmd_buffer += "\n";
        //buffer the message to reduce traffic
        if(get_current_time() - now > 5){
            if(cmd_buffer == cmd && !first_cmd_wait){
                first_cmd_wait = true;
                continue;
            }
            socket_send->send(string_to_message(cmd_buffer));
            cmd_buffer = "";
            first_cmd_wait = true;
        }
        now = get_current_time();
    }
}

int run_worker(){
    // std::unique_ptr <asylo::SigningKey> signing_key(std::move(asylo::EcdsaP256Sha256SigningKey::CreateFromPem(
    //         signing_key_pem)).ValueOrDie());
    // asylo::CleansingVector<uint8_t> serialized_signing_key;
    // ASSIGN_OR_RETURN(serialized_signing_key,
    //                  signing_key->SerializeToDer());

    std::vector <std::thread> worker_threads;
    int num_threads = 4; //# of worker = num_threads - 2
    //worker_threads.push_back(std::thread(thread_run_zmq_router, 0));

    for (unsigned thread_id = START_CLIENT_ID; thread_id < num_threads; thread_id++) {
        //unsigned thread_id = 2;
        Asylo_SGX* sgx = new Asylo_SGX( std::to_string(thread_id+2));
        sgx->init();
        sleep(2);
        worker_threads.push_back(std::thread(thread_start_fake_client, sgx));
	//worker_threads.push_back(std::thread(thread_start_coordinator, sgx));
        worker_threads.push_back(std::thread(thread_run_zmq_js_client, thread_id+2, sgx));
        sleep(1);
    }
    sleep(1000);

    return 0;
}

int run_sync_server(){
    // std::unique_ptr <asylo::SigningKey> signing_key(std::move(asylo::EcdsaP256Sha256SigningKey::CreateFromPem(
    //         signing_key_pem)).ValueOrDie());
    // asylo::CleansingVector<uint8_t> serialized_signing_key;
    // ASSIGN_OR_RETURN(serialized_signing_key,
    //                  signing_key->SerializeToDer());

    std::vector <std::thread> worker_threads;
    int thread_id = 1;
    worker_threads.push_back(std::thread(thread_run_zmq_router, 0));
    Asylo_SGX* sgx = new Asylo_SGX( std::to_string(thread_id));
    sgx->init();
    sleep(2);
    // worker_threads.push_back(std::thread(thread_run_zmq_js_client, thread_id, sgx));
    // worker_threads.push_back(std::thread(thread_start_coordinator, sgx));

    sleep(1000);
    return 0;
}


// This is the function that the enclave will call back into to
// print a message.
void host_helloworld()
{
    fprintf(stdout, "Enclave called into host to print: Hello World!\n");
}

int main(int argc, char* argv[])
{
    char** argv_parse = const_cast<char**>(argv);
#ifdef GFLAGS_NAMESPACE
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv_parse, true);
#else
    google::ParseCommandLineFlags(&argc, &argv_parse, true);
#endif

    uint32_t mode = FLAGS_mode;

    switch (mode)
    {
        case RUN_BOTH_CLIENT_AND_SERVER:
            run_client_and_router();
            break;
        case RUN_CLIENT_ONLY:
            run_clients_only();
            break;
            //        case LISTENER_MODE:
            //            run_listener();
            //            break;
            //        case COORDINATOR_MODE:
            //            run_mpl_coordinator();
            //            break;
        case JS_MODE:
            run_js();
            break;
        case USER_MODE:
            LOG(INFO) << "running in user mode";
            run_user();
            break;
        case COORDINATOR_MODE:
            LOG(INFO) << "running in coordinator mode";
            run_local_dispatcher();
            break;
        case WORKER_MODE:
            LOG(INFO) << "running in worker mode";
            run_worker();
            break;
        case SYNC_SERVER_MODE:
            LOG(INFO) << "running sync server";
            run_sync_server();
        case CAPSULEDB_MODE:
            LOG(INFO) << "running CapsuleDB server";
            // run_capsuleDB();
            break;
        default:
            printf("Mode %d is incorrect\n", mode);
            exit(-1);
    }

    oe_result_t result;
    int ret = 1;
    oe_enclave_t* enclave = NULL;

    uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;
    if (check_simulate_opt(&argc, argv))
    {
        flags |= OE_ENCLAVE_FLAG_SIMULATE;
    }

    if (argc != 2)
    {
        fprintf(
            stderr, "Usage: %s enclave_image_path [ --simulate  ]\n", argv[0]);
        goto exit;
    }

    // Create the enclave
    result = oe_create_helloworld_enclave(
        argv[1], OE_ENCLAVE_TYPE_AUTO, flags, NULL, 0, &enclave);
    if (result != OE_OK)
    {
        fprintf(
            stderr,
            "oe_create_helloworld_enclave(): result=%u (%s)\n",
            result,
            oe_result_str(result));
        goto exit;
    }

    // Call into the enclave
    result = enclave_helloworld(enclave);
    if (result != OE_OK)
    {
        fprintf(
            stderr,
            "calling into enclave_helloworld failed: result=%u (%s)\n",
            result,
            oe_result_str(result));
        goto exit;
    }

    ret = 0;

exit:
    // Clean up the enclave if we created one
    if (enclave)
        oe_terminate_enclave(enclave);

    return ret;
}
