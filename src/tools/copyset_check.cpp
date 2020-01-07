/*
 * Project: curve
 * Created Date: 2019-10-30
 * Author: charisu
 * Copyright (c) 2018 netease
 */
#include "src/tools/copyset_check.h"

DEFINE_bool(detail, false, "list the copyset detail or not");
DEFINE_uint32(logicalPoolId, 0, "logical pool id of copyset");
DEFINE_uint32(copysetId, 0, "copyset id");
DEFINE_uint32(chunkserverId, 0, "chunkserver id");
DEFINE_string(chunkserverAddr, "", "if specified, chunkserverId is not required");  // NOLINT
DEFINE_uint32(serverId, 0, "server id");
DEFINE_string(serverIp, "", "server ip");

namespace curve {
namespace tool {

int CopysetCheck::RunCommand(const std::string& command) {
    int res = 0;
    if (command == "check-copyset") {
        // 检查某个copyset的状态
        if (FLAGS_logicalPoolId == 0 || FLAGS_copysetId == 0) {
            std::cout << "logicalPoolId AND copysetId should be specified!"
                      << std::endl;
            return -1;
        }
        return CheckCopyset();
    } else if (command == "check-chunkserver") {
        // 检查某个chunkserver上的所有copyset
        if (FLAGS_chunkserverAddr.empty() && FLAGS_chunkserverId == 0) {
            std::cout << "chunkserverId OR chunkserverAddr should be secified!"
                      << std::endl;
            return -1;
        }
        if (!FLAGS_chunkserverAddr.empty() && FLAGS_chunkserverId != 0) {
            std::cout << "Only one of chunkserverId OR "
                         "chunkserverAddr should be secified!" << std::endl;
            return -1;
        }
        return CheckChunkServer();
    } else if (command == "check-server") {
        if (FLAGS_serverIp.empty() && FLAGS_serverId == 0) {
            std::cout << "serverId OR serverIp should be secified!"
                      << std::endl;
            return -1;
        }
        if (!FLAGS_serverIp.empty() && FLAGS_serverId != 0) {
            std::cout << "Only one of serverId OR serverIp should be secified!"
                      << std::endl;
            return -1;
        }
        return CheckServer();
    } else if (command == "check-cluster") {
        return CheckCluster();
    } else {
        PrintHelp(command);
        return -1;
    }
}

int CopysetCheck::CheckCopyset() {
    int res = core_->CheckOneCopyset(FLAGS_logicalPoolId, FLAGS_copysetId);
    if (res == 0) {
        std::cout << "Copyset is healthy!" << std::endl;
    } else {
        std::cout << "Copyset is not healthy!" << std::endl;
    }
    if (FLAGS_detail) {
        std::cout << core_->GetCopysetDetail() << std::endl;
        auto serviceExceptionChunkServers =
                        core_->GetServiceExceptionChunkServer();
        if (!serviceExceptionChunkServers.empty()) {
            std::ostream_iterator<std::string> out(std::cout, ", ");
            std::cout << "service-exception chunkservers (total: "
                      << serviceExceptionChunkServers.size() << "): {";
            std::copy(serviceExceptionChunkServers.begin(),
                            serviceExceptionChunkServers.end(), out);
            std::cout << "}" << std::endl;
        }
    }
    return res;
}

int CopysetCheck::CheckChunkServer() {
    int res = 0;
    if (FLAGS_chunkserverId > 0) {
        res = core_->CheckCopysetsOnChunkServer(FLAGS_chunkserverId);
    } else {
        res = core_->CheckCopysetsOnChunkServer(FLAGS_chunkserverAddr);
    }
    if (res == 0) {
        std::cout << "ChunkServer is healthy!" << std::endl;
    } else {
        std::cout << "ChunkServer is not healthy!" << std::endl;
    }
    PrintStatistic();
    if (FLAGS_detail) {
        PrintDetail();
    }
    return res;
}

int CopysetCheck::CheckServer() {
    std::vector<std::string> unhealthyCs;
    int res = 0;
    if (FLAGS_serverId > 0) {
        res = core_->CheckCopysetsOnServer(FLAGS_serverId, &unhealthyCs);
    } else {
        res = core_->CheckCopysetsOnServer(FLAGS_serverIp, &unhealthyCs);
    }
    if (res == 0) {
        std::cout << "Server is healthy!" << std::endl;
    } else {
        std::cout << "Server is not healthy!" << std::endl;
    }
    PrintStatistic();
    if (FLAGS_detail) {
        PrintDetail();
        std::ostream_iterator<std::string> out(std::cout, ", ");
        std::cout << "unhealthy chunkserver list (total: "
                  << unhealthyCs.size() <<"): {";
        std::copy(unhealthyCs.begin(), unhealthyCs.end(), out);
        std::cout << "}" << std::endl;
    }
    return res;
}

int CopysetCheck::CheckCluster() {
    int res = core_->CheckCopysetsInCluster();
    if (res == 0) {
        std::cout << "Cluster is healthy!" << std::endl;
    } else {
        std::cout << "Cluster is not healthy!" << std::endl;
    }
    PrintStatistic();
    if (FLAGS_detail) {
        PrintDetail();
    }
    return res;
}

void CopysetCheck::PrintHelp(const std::string& command) {
    std::cout << "Example: " << std::endl << std::endl;
    if (command == "check-copyset") {
         std::cout << "curve_ops_tool check-copyset -mdsAddr=127.0.0.1:6666 "
         "-logicalPoolId=2 -copysetId=101 [-margin=1000]" << std::endl << std::endl;  // NOLINT
    } else if (command == "check-chunkserver") {
        std::cout << "curve_ops_tool check-chunkserver -mdsAddr=127.0.0.1:6666 "
        "-chunkserverId=1 [-margin=1000]" << std::endl;
        std::cout << "curve_ops_tool check-chunkserver -mdsAddr=127.0.0.1:6666 "
        "-chunkserverAddr=127.0.0.1:8200 [-margin=1000]" << std::endl << std::endl;  // NOLINT
    } else if (command == "check-server") {
        std::cout << "curve_ops_tool check-server -mdsAddr=127.0.0.1:6666 -serverId=1 [-margin=1000]" << std::endl;  // NOLINT
        std::cout << "curve_ops_tool check-server -mdsAddr=127.0.0.1:6666 -serverIp=127.0.0.1 [-margin=1000]" << std::endl;  // NOLINT
    } else if (command == "check-cluster") {
        std::cout << "curve_ops_tool check-cluster -mdsAddr=127.0.0.1:6666 [-margin=1000] [-operatorMaxPeriod=30] [-checkOperator]" << std::endl << std::endl;  // NOLINT
    } else {
        std::cout << "Command not supported! Supported commands: " << std::endl;
        std::cout << "check-copyset" << std::endl;
        std::cout << "check-chunkserver" << std::endl;
        std::cout << "check-server" << std::endl;
        std::cout << "check-cluster" << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Standard of healthy is no copyset in the following state:" << std::endl;  // NOLINT
    std::cout << "1、copyset has no leader" << std::endl;
    std::cout << "2、number of replicas less than expected" << std::endl;
    std::cout << "3、some replicas not online" << std::endl;
    std::cout << "4、installing snapshot" << std::endl;
    std::cout << "5、gap of log index between peers exceed margin" << std::endl;
    std::cout << "6、for check-cluster, it will also check whether the mds is scheduling if -checkOperator specified"  // NOLINT
                "(if no operators in operatorMaxPeriod, it considered healthy)" << std::endl;  // NOLINT
    std::cout << "By default, if the number of replicas is less than 3, it is considered unhealthy, "   // NOLINT
                 "you can change it by specify -replicasNum" << std::endl;
    std::cout << "The order is sorted by priority, if the former is satisfied, the rest will not be checked" << std::endl;  // NOLINT
}


void CopysetCheck::PrintStatistic() {
    const auto& statistics = core_->GetCopysetStatistics();
    std::cout << "total copysets: " << statistics.totalNum
              << ", unhealthy copysets: " << statistics.unhealthyNum
              << ", unhealthy_ratio: "
              << statistics.unhealthyRatio * 100 << "%" << std::endl;
}

void CopysetCheck::PrintDetail() {
    auto copysets = core_->GetCopysetsRes();
    std::ostream_iterator<std::string> out(std::cout, ", ");
    std::cout << std::endl;
    std::cout << "unhealthy copysets statistic: {";
    int i = 0;
    for (const auto& item : copysets) {
        if (item.first == kTotal) {
            continue;
        }
        if (i != 0) {
            std::cout << ", ";
        }
        i++;
        std::cout << item.first << ": ";
        std::cout << item.second.size();
    }
    std::cout << "}" << std::endl;
    std::cout << "unhealthy copysets list: " << std::endl;
    for (const auto& item : copysets) {
        if (item.first == kTotal) {
            continue;
        }
        std::cout << item.first << ": ";
        PrintCopySet(item.second);
    }
    std::cout << std::endl;
    // 打印offline的chunkserver，这里不能严格说它offline，
    // 有可能是chunkserver起来了但无法提供服务,所以叫Service-exception
    auto serviceExceptionChunkServers =
                        core_->GetServiceExceptionChunkServer();
    std::cout << "service-exception chunkserver list (total: "
              << serviceExceptionChunkServers.size() <<"): {";
    std::copy(serviceExceptionChunkServers.begin(),
                        serviceExceptionChunkServers.end(), out);
    std::cout << "}" << std::endl << std::endl;
}

void CopysetCheck::PrintCopySet(const std::set<std::string>& set) {
    std::cout << "{";
    for (auto iter = set.begin(); iter != set.end(); ++iter) {
        if (iter != set.begin()) {
            std::cout << ",";
        }
        std::string gid = *iter;
        uint64_t groupId;
        if (!curve::common::StringToUll(gid, &groupId)) {
            std::cout << "parse group id fail: " << groupId << std::endl;
            continue;
        }
        PoolIdType lgId = groupId >> 32;
        CopySetIdType csId = groupId & (((uint64_t)1 << 32) - 1);
        std::cout << "(grouId: " << gid << ", logicalPoolId: "
                  << std::to_string(lgId) << ", copysetId: "
                  << std::to_string(csId) << ")";
    }
    std::cout << "}" << std::endl;
}
}  // namespace tool
}  // namespace curve