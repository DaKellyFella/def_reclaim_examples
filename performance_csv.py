import sys, os, glob, re

import matplotlib.ticker as plticker
import matplotlib.pyplot as plt

from textwrap import wrap

def mean(numbers):
    return float(sum(numbers)) / max(len(numbers), 1)

# A map translating from the structure name to its 
# [category / nice name, lang, graphing style, and graphing colour]
structure_map = {
  'fhsl_lf' : {
    'structure_category' : 'Skip List',
    'lang': 'DEF'
  },
  'c_fhsl_lf' : {
    'structure_category' : 'Skip List',
    'lang': 'C'
  },
  'bt_lf' : {
    'structure_category' : 'Binary Tree',
    'lang': 'DEF'
  },
  'c_bt_lf' : {
    'structure_category' : 'Binary Tree',
    'lang': 'C'
  },
  'mm_ht' : {
    'structure_category' : 'Hash Table',
    'lang': 'DEF'
  },
  'c_mm_ht' : {
    'structure_category' : 'Hash Table',
    'lang': 'C'
  },
  'lj_pq' : {
    'structure_category' : 'Linden Jonsson Queue',
    'lang': 'DEF'
  },
  'c_lj_pq' : {
    'structure_category' : 'Linden Jonsson Queue',
    'lang': 'C'
  },
  'spray' : {
    'structure_category' : 'Spray List',
    'lang': 'DEF'
  },
  'c_spray' : {
    'structure_category' : 'Spray List',
    'lang': 'C'
  }
}

lang_map = {
  'DEF leaky' : {
    'graph_style' : '*', 
    'graph_colour': 'blue'
  },
  'C leaky' : {
    'graph_style' : 's', 
    'graph_colour': 'crimson'
  },
  'DEF retire' : {
    'graph_style' : '^', 
    'graph_colour': 'green'
  },
}

def collate_threads(data):
  thread_ops = {}
  for datum in data:
    (threads, ops) = datum
    if not(threads in thread_ops):
      thread_ops[threads] = []
    thread_ops[threads].append(ops)
  return thread_ops

def create_line_plots(config, perf_results):
  plt.rc('font', size = 22);
  plt.rc('legend', fontsize = 16);
  plt.rc('xtick', labelsize = 16);
  plt.rc('ytick', labelsize = 16);

  fig = plt.figure(figsize = (10, 6), dpi = 100)
  # plt.rcParams['axes.facecolor'] = (230.0 / 255.0, 1, 1)

  (structure_name, lang_results, filename) = perf_results[config]
  plt.title(config)
  plt.ylabel(r'Total Ops / $\mu$s')
  plt.xlabel('Thread Count')
  # plt.xscale('linear')
  plt.autoscale(enable=True, axis='both', tight=None)
  plt.grid(b=True, which='major', color='#ccbc8a', linestyle='-')
  legends_list = []
  max_ops, min_ops = 0, 0
  for lang_config in sorted(lang_results.keys()):
    (policy, raw_name, data) = lang_results[lang_config]
    thread_ops = collate_threads(data)
    y_ticks = [0]
    y_error = [0]
    x_ticks = [0]
    thread_points = [0]

    for threads in range(1, max(thread_ops) + 1):
      if threads in thread_ops:
        y_ticks.append(mean(thread_ops[threads]))
        y_error.append(max(thread_ops[threads]) - min(thread_ops[threads]))
        thread_points.append(threads)
    style_info = lang_map[lang_config]
    max_ops = max(max(y_ticks), max_ops)
    min_ops = min(min(y_ticks), min_ops)
    legends_list.append(
      plt.errorbar(x = range(len(thread_points)), y = y_ticks, linewidth=2,
        label = lang_config, color = style_info['graph_colour'], marker = style_info['graph_style'], markersize=7))
  # plt.gca().set_facecolor((0, 0, 0.25))
  if structure_name == 'Spray List' or structure_name == 'Linden Jonsson Queue':
    plt.legend(loc='best', fancybox=True, shadow=True, handles = legends_list, ncol = 1)
  else:
    plt.legend(loc='upper left', fancybox=True, shadow=True, handles = legends_list, ncol = 1)
    
  plt.ylim(ymin = 0)
  x_ticks = [0, 1, 2, 3, 4, 9] + range(18, 153, 18)
  x_ranges = [0, 1, 2, 3, 4, 5] + range(6, len(x_ticks) * 2, 2)
  plt.gca().xaxis.set_ticks(x_ranges) #set the ticks to be a
  plt.gca().xaxis.set_ticklabels(x_ticks) # change the ticks' names to x
  # plt.gca().tick_params(axis='both', which='major', pad=15)
  plt.xlim(0, len(thread_points))
  # loc = plticker.MultipleLocator(base=2) # this locator puts ticks at regular intervals
  # plt.gca().xaxis.set_major_locator(loc)
  fig.savefig('./figures/' + filename + '.pdf', bbox_inches='tight')

def create_calibrating_bar_plots(set_results, pqueue_results):
  # All structures into one bar plot.
  leaky_def_results = {}
  def_results = {}
  c_results = {}
  for config in set_results:
    (structure_name, lang_results, _) = set_results[config]
    for lang_config in lang_results:
      (policy, raw_name, data) = lang_results[lang_config]
      thread_ops = collate_threads(data)
      lang = structure_map[raw_name]['lang']
      if lang == 'DEF' and policy == 'leaky':
        leaky_def_results[structure_name] = mean(thread_ops[1])
      elif lang == 'DEF' and policy == 'retire':
        def_results[structure_name] = mean(thread_ops[1])
      else:
        c_results[structure_name] = mean(thread_ops[1])
  
  for config in pqueue_results:
    (structure_name, lang_results, _) = pqueue_results[config]
    for lang_config in sorted(lang_results.keys()):
      (policy, raw_name, data) = lang_results[lang_config]
      thread_ops = collate_threads(data)
      lang = structure_map[raw_name]['lang']
      if lang == 'DEF' and policy == 'leaky':
        leaky_def_results[structure_name] = mean(thread_ops[1])
      elif lang == 'DEF' and policy == 'retire':
        def_results[structure_name] = mean(thread_ops[1])
      elif lang == 'C':
        c_results[structure_name] = mean(thread_ops[1])
  structures = []
  leaky_def_numbers = []
  def_numbers = []
  c_numbers = []
  for structure in leaky_def_results:
    structures.append('\n'.join(wrap(structure, 12)))
    leaky_def_numbers.append(leaky_def_results[structure])
    def_numbers.append(def_results[structure])
    c_numbers.append(c_results[structure])
  def_norms = []
  leaky_def_norms = []
  c_norms = []
  for (def_res, leaky_def_res, c_res) in zip(def_numbers, leaky_def_numbers, c_numbers):
    def_norms.append((def_res / c_res) * 100.0)
    leaky_def_norms.append((leaky_def_res / c_res) * 100.0)
    c_norms.append(100.0)

  plt.rc('font', size = 22);
  plt.rc('legend', fontsize = 22);
  plt.rc('xtick', labelsize = 22);
  plt.rc('ytick', labelsize = 22);

  fig = plt.figure(figsize = (16, 9), dpi = 100)
  width = 0.25
  ind = range(len(def_norms))
  off_ind = [x +width for x in ind]
  plt.title('Normalised data-structure performance.')
  ax = plt.gca()
  ax.yaxis.grid(b=True, which='major', color='#ccbc8a', linestyle='-', zorder = 1)
  bar1 = ax.bar(ind, def_norms, width, color=lang_map['DEF retire']['graph_colour'], zorder = 0)
  bar2 = ax.bar(off_ind, leaky_def_norms, width, color=lang_map['DEF leaky']['graph_colour'], zorder = 0)
  # loc = plticker.MultipleLocator(base=20) # this locator puts ticks at regular intervals
  # ax.yaxis.set_major_locator(loc)
  ax.set_ylim(0,140)
  # ax.set_xlim(-width,len(ind)+width)
  ax.set_xlim(-width * 2,len(ind))
  ax.set_ylabel('Relative Performance %')
  ax.set_title('Single Core: DEF relative to C')
  ax.set_xticks([x + (width / 2) for x in ind])
  xtickNames = ax.set_xticklabels(structures)
  ax.axhline(y = 100, lw = 4, color = 'red')
  plt.setp(xtickNames, rotation=45, fontsize=18)

  ## add a legend
  ax.legend( [bar1, bar2], ['DEF retire', 'DEF leaky'] )
  plt.savefig('./figures/RelativePerf.pdf', bbox_inches='tight')

def parse_file(key, data):
  key_file = key
  data_file = data

  keys = {}
  with open(key_file, 'r') as open_file:
    for key_line in open_file:
      for idx, key in enumerate(key_line.split(",")):
        keys[key.strip()] = idx

  raw_results = []
  with open(data_file, 'r') as open_file:
    for result in open_file:
      raw_results.append([x.strip() for x in result.split(",")])
  
  # Collate all the data.
  perf_results = {}
  for result in raw_results:
    # Get config.
    structure_key = result[keys['benchmark']]
    structure_m = structure_map[structure_key]
    structure_category = structure_m['structure_category']

    config = structure_category
    filename = structure_category.replace(" ", "")
    if 'update_rate' in keys:
      config += ' w update rate: ' + result[keys['update_rate']] + '%'
      if result[keys['update_rate']] == '10':
        filename += 'Light'
      elif result[keys['update_rate']] == '20':
        filename += 'Medium'

    if not(config in perf_results):
      perf_results[config] = (structure_category, {}, filename)
    _, category_results_map, _ = perf_results[config]

    policy = result[keys['policy']]
    lang_config = structure_m['lang'] + ' ' + policy

    if not(lang_config in category_results_map):
      category_results_map[lang_config] = (policy, structure_key, [])
    _, _, lang_policy_results = category_results_map[lang_config]
    # Division to make into microseconds
    lang_policy_results.append((int(result[keys['threads']]), float(result[keys['ops/sec']]) / 1000000))
  return perf_results


def main():
  if not os.path.exists('figures'):
    os.makedirs('figures')
  set_results = parse_file('set_keys.csv', 'set_data.csv')
  plt.rcParams['axes.facecolor'] = (1, 1, 230.0 / 255.0)
  # plt.rcParams['savefig.facecolor'] = (227.0 / 255.0, 242.0 / 255.0, 231.0 / 255.0)
  # plt.rcParams['savefig.facecolor'] = (178.0 / 255.0, 192.0 / 255.0, 181.0 / 255.0) ash grey
  # plt.rcParams['savefig.facecolor'] = (227.0 / 255.0, 242.0 / 255.0, 231.0 / 255.0)
  plt.rcParams['savefig.facecolor'] = 'white'
  for config in set_results:
    create_line_plots(config, set_results)
  
  pqueue_results = parse_file('pqueue_keys.csv', 'pqueue_data.csv')
  for config in pqueue_results:
    create_line_plots(config, pqueue_results)
  
  create_calibrating_bar_plots(set_results, pqueue_results)
    
   
if __name__ == '__main__':
  main()
