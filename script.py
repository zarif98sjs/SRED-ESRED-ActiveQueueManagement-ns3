import xml.etree.ElementTree as ET
import os
import pandas as pd

# monitors = ["wpan-sourceCount1.flowmonitor", "wpan-sourceCount2.flowmonitor", 
#             "wpan-sourceCount4.flowmonitor", "wpan-sourceCount8.flowmonitor"]
            
monitors = ["wpan-sourceCount1.flowmonitor"]
        
m_dict = {"wpan-sourceCount1.flowmonitor": '1 Source'}
          

dirs = ['']

d_dict = {'': 0}

df = pd.DataFrame(columns=['Error Rate', '1 Source'])

for d in dirs:
    row = {'Error Rate': d_dict[d]}

    for m in monitors:
        path = os.path.join(os.getcwd(), d, m)
        tree = ET.parse(path)
        root = tree.getroot()

        for child in root:
            throughput = 0
            for i, flow in enumerate(child):
                if i>= len(child)/2:
                    break
                throughput += int(flow.attrib['rxBytes'])*8/(100*1000)
            row[ m_dict[m] ] = round(throughput/(len(child)/2))
            break

    df = df.append(pd.Series(row), ignore_index=True)

df.to_csv("throughput.csv", index=False)


df = pd.DataFrame(columns=['Error Rate', '1 Source'])

for d in dirs:
    row = {'Error Rate': d_dict[d]}

    for m in monitors:
        path = os.path.join(os.getcwd(), d, m)
        tree = ET.parse(path)
        root = tree.getroot()

        for child in root:
            dropRatio = 0
            for i, flow in enumerate(child):
                if i>= len(child)/2:
                    break
                dropRatio += (int(flow.attrib['txPackets'])-int(flow.attrib['rxPackets'])) \
                                / int(flow.attrib['rxPackets'])
            row[ m_dict[m] ] = round(dropRatio/(len(child)/2) * 100, 3)
            break

    df = df.append(pd.Series(row), ignore_index=True)

df.to_csv("droprate.csv", index=False)