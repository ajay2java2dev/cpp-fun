#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

//https://www.geeksforgeeks.org/distance-vector-routing-dvr-protocol/

//Info on the implementation.
/*
 * Information kept by DV Router -
 *  1) Each router has an ID.
 *  2) Associated with each link connected to a router, there is a link cost(static or dynamic)
 *  3) Intermediate hops
 *
 *  Distance Vector Table Initialization -
 *  1) Distance to itself = 0
 *  2) Distance to ALL other routers = infinity number
 *
 *  Algorithm -
 *  1) Router transmits its distance vector to each of its neighbors in a routing packet.
 *  2) Each router receives and save the most recently received distance vector from each of its neighbours.
 *  3) A router recalculates its distance vector when:
 *      - it receives a distance vector from a neighbor containing different information than before.
 *      - it discovers that a link to a neighbor has gone down.
 *
 *      DV CALCULATION - MINIMIZING THE COST TO EACH DESTINATION
 *
 *      Dx (y) = Estimate of least cost from x to y
 *      C (x,v) = Node x knows cost to each neighbor v
 *      Dx = [Dx (y): y belongs to N] = Node x maintains distance vector
 *      Node x also maintains its neighbor's distance vectors.
 *          - For each neighbor v, x maintains Dv = [Dv(y) : y belongs to N]
 */

#define M 255
#define N 255
#define M 255

struct routers {
    unsigned dist [M];
    unsigned from [N];
}rt_full[M];

int *node_list;


//lets modify the topology
void calculateDistanceVector(int num_nodes, int cost_matrix[M][N]) {

    int count = 0;
    do {
        for (int i = 0; i < num_nodes; i++) {
            int i_node = node_list [i];
            for (int j =0; j < num_nodes; j++) {
                int j_node = node_list [j];
                for (int k = 0; k < num_nodes; k++) {
                    int k_node = node_list [k];

                    if (rt_full[i_node].dist[j_node] > cost_matrix[i_node][k_node] + rt_full[k_node].dist[j_node]) {

                        printf("\n##################");
                        printf("\ncount = %d",count);
                        printf("\nrt_full[i].dist[j]  == %d", rt_full[i_node].dist[j_node]);
                        printf("\ncost_matrix[i][k]  == %d", cost_matrix[i_node][k_node]);
                        printf("\nrt_full[k].dist[j]  == %d", rt_full[k_node].dist[j_node]);
                        printf("\n##################");

                        rt_full[i_node].dist[j_node] = rt_full[i_node].dist[k_node] + rt_full[k_node].dist[j_node];
                        rt_full[i_node].from[j_node] = k_node;
                        count ++;

                    }
                }
            }
        }
    } while (count <= 1);

    for (int i = 0; i < num_nodes ; i++) {
        for (int j = 0; j < num_nodes; j++) {

            int i_node = node_list [i];
            int j_node = node_list [j];

            printf("\n%d = %d --> rt_full[i].dist[j]  == %d, rt_full[i_node].dist[j_node] =%d",i_node,j_node,
                    rt_full[i_node].dist[j_node],rt_full[i_node].from[j_node]);
        }
    }

}

//-- symmetric 3 top view
/*
 * [0   3   4
 * 3    0   10
 * 4    10  0]
 */
void constructTopologyNetwork(int cost_matrix[M][N]) {

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            rt_full[i].dist[j] = cost_matrix[i][j];
            rt_full[i].from[j] = j;
            printf("\n%d from %d ", rt_full[i].dist[j], rt_full[i].from[j]);
        }
    }

}

int main() {

    int n,i,j,k,count = 0;

    int cost_matrix[M][N];
    int num_nodes = 3;
    node_list = (int*) malloc(sizeof (int*));

    for (int i = 0; i < M; i++) {

        node_list[i] = -1;

        for (int j = 0; j < N; j++) {
            cost_matrix[i][i] = 0;
            cost_matrix[i][j] = INT_MAX;
        }
    }

    cost_matrix[0][1] = 10;
    cost_matrix[0][2] = 20;
    cost_matrix[1][0] = 10;
    cost_matrix[1][2] = 50;
    cost_matrix[2][0] = 20;
    cost_matrix[2][1] = 50;
    cost_matrix[250][1] = 10;
    cost_matrix[1][250] = 10;
    cost_matrix[200][250] = 15;
    cost_matrix[250][200] = 15;
    node_list[0]=0;
    node_list[1]=1;
    node_list[2]=2;
    node_list[3]=250;
    node_list[4]=200;

    constructTopologyNetwork(cost_matrix); // 255 * 255 topology
    calculateDistanceVector(5,cost_matrix);
}