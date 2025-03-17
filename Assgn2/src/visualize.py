import os, sys
import matplotlib
matplotlib.use('Agg')
from matplotlib import style
import matplotlib.pyplot as plt
import networkx as nx
import pydot
from networkx.drawing.nx_pydot import graphviz_layout
from os.path import dirname, abspath
import pandas as pd
import numpy as np
import warnings

warnings.filterwarnings("ignore")

def draw_blockchain(G, filename):
    if len(G) == 0:
        G.add_node(1)
    pos = graphviz_layout(G, prog="dot")
    pos = {k: (-y, x) for k, (x, y) in pos.items()}
    fig = plt.figure(dpi=300)
    n = len(G)
    node_size = 7000 // n
    font_size = min(8, 700 // n)
    edge_width = min(1.0, font_size / 4)
    nx.draw(G, pos, 
        node_size=node_size, 
        font_size=font_size, 
        arrowsize=font_size, 
        width=edge_width, 
        node_color='blue', 
        with_labels=True, 
        font_color="white"
    )
    plt.savefig(filename, bbox_inches='tight')
    plt.close()

def draw_peer_network(file, adversary=True):
    with open(file, 'r') as fp:
        lines = [l.strip() for l in fp.readlines() if l.strip()]

    # Extract malicious nodes (assuming they are at the beginning)
    malicious_nodes = set()
    edges = []
    for line in lines:
        parts = line.split()
        if len(parts) == 1:  # Single number = malicious node
            malicious_nodes.add(int(parts[0]))
        else:  # Two numbers = edge
            edges.append((int(parts[0]), int(parts[1])))

    # Create the graph
    G = nx.Graph(edges)

    # Assign colors: red for malicious, blue for normal nodes
    colours = ['darkred' if v in malicious_nodes else '#1f78b4' for v in G.nodes()]

    # Graph layout
    pos = graphviz_layout(G, prog="circo")
    filename = file.replace('_edgelist.txt', '_img.png')

    # Draw graph
    nx.draw(G, pos, node_color=colours, with_labels=True, font_color="white")
    
    # Save and close
    plt.savefig(filename, bbox_inches='tight')
    plt.close()



base = os.path.join(dirname(dirname(abspath(__file__))), 'outputs')
network_file = os.path.join(base, 'peer_network_edgelist.txt')
draw_peer_network(network_file)
overlay_file = os.path.join(base, 'overlay_network_edgelist.txt')
draw_peer_network(overlay_file)

peer = int(sys.argv[1]) if len(sys.argv) > 1 else -1
num_peers = len(os.listdir(os.path.join(base, 'blockchains')))
peer = (peer % num_peers + num_peers) % num_peers

for node in range(num_peers):
    peer = f'{node + 1}.txt'
    path = os.path.join(base, "blockchains", peer)
    with open(path, 'r') as fp:
        edges = [l.strip().split() for l in fp.readlines() if l.strip()]
        edges = [(int(u), int(v)) for u, v in edges]
    G = nx.DiGraph(edges)
    path2 = os.path.join(base,"blockchain_images/")
    filename = path2 + f'{node+1}_img.png'
    draw_blockchain(G, filename)
