#ifndef NODES_H
#define NODES_H

#include <stdint.h>
#include <stdio.h>

struct Node {
    char    *ip;
    char    *key;
    uint16_t udp_port;
    uint16_t tcp_port;
} nodes[] = {
    // clang-format off
    { "tox.initramfs.io", "3F0A45A268367C1BEA652F258C85F4A66DA76BCAA667A49E770BCC4917AB6A25", 33445, 3389 }, // initramfs
    { "tox.abilinski.com", "10C00EB250C3233E343E2AEBA07115A5C28920E9C8D29492F6D00B29049EDC7E", 33445, 33445 }, // AnthonyBilinski
    { "tox.plastiras.org", "8E8B63299B3D520FB377FE5100E65E3322F7AE5B20A0ACED2981769FC5B43725", 33445, 33445 }, // Tha_14
    { "172.104.215.182", "DA2BD927E01CD05EBCC2574EBE5BEBB10FF59AE0B2105A7D1E2B40E49BB20239", 33445, 33445 }, // zero-one
    { NULL, NULL, 0, 0 }
    // clang-format on
};

#endif
