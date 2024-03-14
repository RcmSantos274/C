import java.io.*;
import java.util.*;

public class CacheSimulator {

    // Constantes
    public static final float MAX_UINT = 4294967295L;

    // Classes auxiliares
    static class Bloco {
        int val;
        int tag;

        public Bloco() {
            this.val = 0;
            this.tag = 0;
        }
    }

    static class Conj {
        Bloco[] blocos;

        public Conj(int assoc) {
            this.blocos = new Bloco[assoc];
            for (int i = 0; i < assoc; i++) {
                this.blocos[i] = new Bloco();
            }
        }
    }

    static class Cache {
        int nsets;
        int bsize;
        int assoc;
        char subst;
        Conj[] conjs;

        public Cache(int nsets, int bsize, int assoc, char subst) {
            this.nsets = nsets;
            this.bsize = bsize;
            this.assoc = assoc;
            this.subst = subst;
            this.conjs = new Conj[nsets];
            for (int i = 0; i < nsets; i++) {
                this.conjs[i] = new Conj(assoc);
            }
        }
    }

    static class CacheStats {
        int acessos;
        int hits;
        int misses;
        int comp_misses;
        int cap_misses;
        int conf_misses;

        public CacheStats() {
            this.acessos = 0;
            this.hits = 0;
            this.misses = 0;
            this.comp_misses = 0;
            this.cap_misses = 0;
            this.conf_misses = 0;
        }
    }

    // Métodos
    public static int reverse_address(int address) {
        int mask = 0xff000000;
        int reversed_address = 0;

        reversed_address |= ((address << 24) & mask);
        reversed_address |= ((address << 8) & (mask >>> (2 * 4)));
        reversed_address |= ((address >>> 8) & (mask >>> (4 * 4)));
        reversed_address |= ((address >>> 24) & (mask >>> (6 * 4)));

        return reversed_address;
    }

    public static void load_addresses(Cache cache, int nsets, int assoc, String file, int numBitsOffset, int numBitsIndex, CacheStats stats) {
        int blockAmount = nsets * assoc;
        int valCount = 0;

        try {
            DataInputStream arq = new DataInputStream(new FileInputStream(file));
            while (true) {
                int address = arq.readInt();
                int tagMissFlag = 1;
                address = reverse_address(address);
                int TAG = address >>> (numBitsOffset + numBitsIndex);
                int IDX = (address >>> numBitsOffset) & ((int) Math.pow(2, numBitsIndex) - 1);
                stats.acessos++;

                if (assoc == 1) { // Mapeamento Direto
                    if (cache.conjs[IDX].blocos[0].val == 1) {
                        if (cache.conjs[IDX].blocos[0].tag == TAG) {
                            stats.hits++;
                        } else {
                            stats.conf_misses++;
                            stats.misses++;
                            cache.conjs[IDX].blocos[0].tag = TAG;
                        }
                    } else {
                        stats.comp_misses++;
                        stats.misses++;
                        cache.conjs[IDX].blocos[0].val = 1;
                        cache.conjs[IDX].blocos[0].tag = TAG;
                    }
                } else { // Conjunto Associativo
                    for (int i = 0; i < assoc; i++) {
                        if (cache.conjs[IDX].blocos[i].val == 1) {
                            if (cache.conjs[IDX].blocos[i].tag == TAG) {
                                stats.hits++;
                                tagMissFlag = 0;
                                break;
                            }
                        } else {
                            stats.comp_misses++;
                            stats.misses++;
                            valCount++;
                            cache.conjs[IDX].blocos[i].val = 1;
                            cache.conjs[IDX].blocos[i].tag = TAG;
                            tagMissFlag = 0;
                            break;
                        }
                    }

                    if (tagMissFlag == 1) {
                        int ranIdx = rand_int(assoc);
                        cache.conjs[IDX].blocos[ranIdx].tag = TAG;
                        stats.misses++;
                        if (nsets == 1) {
                            stats.cap_misses++;
                        } else {
                            if (valCount == blockAmount) {
                                stats.cap_misses++;
                            } else {
                                stats.conf_misses++;
                            }
                        }
                    }

                    arq.close();
                }
            }
        } catch (EOFException e) {
            // reached end of file
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static int rand_int(int max) {
        Random random = new Random();
        return random.nextInt(max);
    }

    public static void print_results(int flag, CacheStats stats) {
        double hit_rate = (double) stats.hits / stats.acessos;
        double miss_rate = (double) stats.misses / stats.acessos;
        double comp_miss_rate = (double) stats.comp_misses / stats.misses;
        double cap_miss_rate = (double) stats.cap_misses / stats.misses;
        double conf_miss_rate = (double) stats.conf_misses / stats.misses;

        if (flag == 0) {
            System.out.println("Acessos: " + stats.acessos);
            System.out.println("Taxa de Hits: " + String.format("%.4f", hit_rate));
            System.out.println("Taxa de Misses: " + String.format("%.4f", miss_rate));
            System.out.println("Taxa de Misses Compulsorios: " + String.format("%.2f", comp_miss_rate));
            System.out.println("Taxa de Misses Capacidade: " + String.format("%.2f", cap_miss_rate));
            System.out.println("Taxa de Misses Conflito: " + String.format("%.2f", conf_miss_rate));
        } else {
            System.out.println(stats.acessos + " " + String.format("%.4f", hit_rate) + " " + String.format("%.4f", miss_rate) + " " + String.format("%.2f", comp_miss_rate) + " " + String.format("%.2f", cap_miss_rate) + " " + String.format("%.2f", conf_miss_rate));
        }
    }

    public static void main(String[] args) {
        if (args.length != 7) {
            System.out.println("Padrao utilizado: <nsets> <bsize> <assoc> <substituição> <flag_saida> <arquivo_de_entrada>");
            return;
        }
        if (args[4].charAt(0) != 'R') {
            System.out.println("Apenas a politica de substituicao RANDOM foi implementada.");
            return;
        }

        CacheStats stats = new CacheStats();
        //Random random = new Random(); // Initializing random number generator

        int nsets = Integer.parseInt(args[1]);
        int bsize = Integer.parseInt(args[2]);
        int assoc = Integer.parseInt(args[3]);
        char subst = args[4].charAt(0);
        int flag_saida = Integer.parseInt(args[5]);
        String file = args[6];

        Cache cache = new Cache(nsets, bsize, assoc, subst);

        int numBitsOffset = (int) Math.log(bsize) / (int) Math.log(2);
        int numBitsIndex = (int) Math.log(nsets) / (int) Math.log(2);
        //int numBitsTag = 32 - numBitsOffset - numBitsIndex;

        load_addresses(cache, nsets, assoc, file, numBitsOffset, numBitsIndex, stats);
        print_results(flag_saida, stats);
    }
}



