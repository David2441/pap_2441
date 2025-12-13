import 'package:flutter/material.dart';
import 'dart:convert';
import 'package:http/http.dart' as http;

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const ProgramaSemanalApp();
  }
}

class ProgramaSemanalApp extends StatelessWidget {
  const ProgramaSemanalApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Programa Semanal',
      theme: ThemeData(primarySwatch: Colors.indigo),
      home: const ProgramaSemanalPage(),
    );
  }
}

class ProgramaSemanalPage extends StatefulWidget {
  const ProgramaSemanalPage({super.key});

  @override
  State<ProgramaSemanalPage> createState() => _ProgramaSemanalPageState();
}

class _ProgramaSemanalPageState extends State<ProgramaSemanalPage> {
  final Map<String, Map<int, Map<String, int>>> _programa = {
    'Segunda-feira': {},
    'Terça-feira': {},
    'Quarta-feira': {},
    'Quinta-feira': {},
    'Sexta-feira': {},
    'Sábado': {},
    'Domingo': {},
  };

  final Map<String, String> _abreviacoes = {
    'Segunda-feira': 'SEG',
    'Terça-feira': 'TER',
    'Quarta-feira': 'QUA',
    'Quinta-feira': 'QUI',
    'Sexta-feira': 'SEX',
    'Sábado': 'SAB',
    'Domingo': 'DOM',
  };

  final List<String> _opcoesMedicamentos = [
    'Paracetamol',
    'Ibuprofeno',
    'Sedatif',
  ];

  List<Map<String, dynamic>> dadosGerados = [];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Programa Semanal'),
      ),
      body: Column(
        children: [
          Expanded(
            child: ListView(
              children: _programa.keys.map((dia) {
                return Card(
                  margin: const EdgeInsets.all(10),
                  child: ExpansionTile(
                    title: Text(
                      dia,
                      style: const TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    children: List.generate(24, (hora) {
                      final atividades = _programa[dia]![hora];
                      final horaFormatada = hora.toString().padLeft(2, '0') + ':00';
                      return ListTile(
                        leading: const Icon(Icons.access_time),
                        title: Text(horaFormatada),
                        subtitle: atividades != null && atividades.isNotEmpty
                            ? Text(
                                atividades.entries
                                    .where((e) => e.value > 0)
                                    .map((e) => '${e.key}: ${e.value}')
                                    .join(', '),
                              )
                            : const Text('Sem medicamentos'),
                        trailing: IconButton(
                          icon: const Icon(Icons.add),
                          onPressed: () {
                            _adicionarAtividade(context, dia, hora);
                          },
                        ),
                      );
                    }),
                  ),
                );
              }).toList(),
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(16.0),
            child: ElevatedButton.icon(
              icon: const Icon(Icons.save),
              label: const Text('Gerar dados'),
              style: ElevatedButton.styleFrom(
                minimumSize: const Size(double.infinity, 50),
              ),
              onPressed: _gerarDados,
            ),
          ),
        ],
      ),
    );
  }

  void _adicionarAtividade(BuildContext context, String dia, int hora) async {
    final Map<String, int> quantidades =
        Map<String, int>.from(_programa[dia]![hora] ?? {
      for (var m in _opcoesMedicamentos) m: 0,
    });

    await showDialog(
      context: context,
      builder: (context) {
        return StatefulBuilder(
          builder: (context, setStateDialog) {
            return AlertDialog(
              title: Text(
                'Indicar comprimidos - $dia ${hora.toString().padLeft(2, '0')}:00',
              ),
              content: SingleChildScrollView(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: _opcoesMedicamentos.map((med) {
                    return Padding(
                      padding: const EdgeInsets.symmetric(vertical: 6),
                      child: Row(
                        children: [
                          Expanded(child: Text(med)),
                          IconButton(
                            icon: const Icon(Icons.remove_circle_outline),
                            onPressed: () {
                              setStateDialog(() {
                                if (quantidades[med]! > 0) {
                                  quantidades[med] = quantidades[med]! - 1;
                                }
                              });
                            },
                          ),
                          Text(
                            quantidades[med].toString(),
                            style: const TextStyle(
                              fontSize: 16,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                          IconButton(
                            icon: const Icon(Icons.add_circle_outline),
                            onPressed: () {
                              setStateDialog(() {
                                quantidades[med] =
                                    (quantidades[med]! + 1).clamp(0, 99);
                              });
                            },
                          ),
                        ],
                      ),
                    );
                  }).toList(),
                ),
              ),
              actions: [
                TextButton(
                  child: const Text('Cancelar'),
                  onPressed: () => Navigator.pop(context),
                ),
                ElevatedButton(
                  child: const Text('Salvar'),
                  onPressed: () {
                    setState(() {
                      _programa[dia]![hora] = quantidades;
                    });
                    Navigator.pop(context);
                  },
                ),
              ],
            );
          },
        );
      },
    );
  }

  Future<void> sendDataToESP32() async {
    try {
      const espUrl = "http://10.154.162.133/data";

      final payload = {"dados": dadosGerados};

      final response = await http.post(
        Uri.parse(espUrl),
        headers: {"Content-Type": "application/json"},
        body: jsonEncode(payload),
      );

      print("Resposta do ESP32: ${response.statusCode} ${response.body}");
    } catch (e) {
      print("Erro a enviar para ESP32: $e");
    }
  }

  void _gerarDados() async {
    dadosGerados.clear();

    _programa.forEach((dia, horas) {
      final abreviacao = _abreviacoes[dia] ?? dia.substring(0, 3).toUpperCase();
      horas.forEach((hora, medicamentos) {
        medicamentos.forEach((nome, qtd) {
          if (qtd > 0) {
            dadosGerados.add({
              "Dia": abreviacao,
              "Hora": hora,
              "Medicamento": nome,
              "Quantidade": qtd,
            });
          }
        });
      });
    });

    // Apenas para verificar no console
    print(dadosGerados);

    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('Dados gerados! Veja o console.'),
        duration: Duration(seconds: 2),
      ),
    );

    await sendDataToESP32();

  }
}
