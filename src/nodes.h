#ifndef NODES_H
#define NODES_H

#include <stdint.h>
#include <stdio.h>

struct Node {
    char *   ip;
    char *   key;
    uint16_t udp_port;
    uint16_t tcp_port;
} nodes[] = {
    //clang-format off
    { "tox.initramfs.io", "3F0A45A268367C1BEA652F258C85F4A66DA76BCAA667A49E770BCC4917AB6A25", 33445, 3389 },   // initramfs
    { "tox.kurnevsky.net", "82EF82BA33445A1F91A7DB27189ECFC0C013E06E3DA71F588ED692BED625EC23", 33445, 33445 }, // kurnevsky
    { "185.14.30.213", "2555763C8C460495B14157D234DD56B86300A2395554BCAE4621AC345B8C1B1B", 443, 0 },           // dvor
    { "nodes.tox.chat", "6FC41E2BD381D37E9748FC0E0328CE086AF9598BECC8FEB7DDF2E440475F300E", 33445, 33445 },    // Impyy
    { "tox.abilinski.com", "10C00EB250C3233E343E2AEBA07115A5C28920E9C8D29492F6D00B29049EDC7E", 33445, 33445 }, // AnthonyBilinski
    { NULL, NULL, 0, 0 }
    //clang-format on
};

#endif
