# sap2000-to-staad-pro-geometry

## El Problema: Pérdida de Tiempo en Conversión Manual
Existe una cantidad considerable de softwares especializados en el cálculo estructural, tales como SAP2000 y Staad. Frecuentemente, la labor de un estructurista es la de revisar el trabajo de un colega, el cual muy posiblemente haya elaborado su modelo con un software diferente del que se tiene disponible, por lo que surge la necesidad de realizar un modelo analítico desde cero, mas aún cuando la única información con la que se cuenta de la estructura analizada es un archivo de SAP2000.

<br>

Recrear modelos de SAP2000 a STAAD de manera manual es **propenso a errores** y consume horas valiosas. Mi objetivo fue desarrollar una herramienta que, automáticamente:

<br>

- **Extraiga datos directamente** del archivo .$2K de SAP2000.  
- **Genere comandos nativos** para STAAD.  

## ¿Qué hace este programa?  
- Convierte **geometría, cargas y soportes** de SAP2000 versión 2022 a STAAD Pro CONNECT EDITION empleando el API OpenStaad.  
- Soporta beams, plates de 4 nodos y cargas puntuales y distribuidas sobre beams y plates.  

## Cómo Funciona el Código

### 1. Lectura del Archivo SAP2000  

El programa analiza el archivo .$2K como texto plano. Aprovechando que la información de las propiedades de la estructura se encuentra organizada en tablas, se buscan patrones clave en su contenido:

<br>

```cpp  
SectionPositions SAP2000Parser::ParseFile(const string& filePath) {
    SectionPositions positions;
...
    string line;
    long lineNumber = 1;

    while (getline(inputFile, line)) {
        replace(line.begin(), line.end(), ',', '.');

        if (line.find("JOINT COORDINATES") != string::npos) {
            positions.ijoint = lineNumber;
        }
        else if (line.find("CONNECTIVITY - FRAME") != string::npos) {
            positions.iconnection = lineNumber;
...
}
        lineNumber++;
    }
    return positions;
}
```
<br>

Después, se realiza el *parsing* de las tablas de acuerdo con el tipo de tabla y la información buscada. Por ejemplo, para crear la matriz de nodos "JOINT COORDINATES":

<br>

```cpp  
vector<Node> SAP2000Parser::ExtractNodes(const string& filePath, long startLine) {
    vector<Node> nodes;
    ifstream inputFile(filePath);
...
    string line;
    long currentLine = 1;
    while (currentLine < startLine && getline(inputFile, line)) {
        currentLine++;
    }
    while (getline(inputFile, line) && !line.empty()) {
...
        if (std::all_of(line.begin(), line.end(), isspace)) break;
...
        try {
            size_t jointPos = line.find("Joint=");
            if (jointPos != string::npos) {
                size_t endPos = line.find(' ', jointPos);
                string jointVal = line.substr(jointPos + 6, endPos - (jointPos + 6));
                node.sapId = stoi(jointVal);
            }
            // Find XorR= value (X coordinate)
            size_t xPos = line.find("XorR=");
            if (xPos != string::npos) {
                size_t endPos = line.find(' ', xPos);
                string xVal = line.substr(xPos + 5, endPos - (xPos + 5));
                node.x = stod(xVal);
            }
...
            if (node.sapId != 0) {
                nodes.push_back(node);
            }
```

<br>

### 2. Generación de Geometría en STAAD.Pro

    Nodos y beams se crean con comandos como AddNode y AddBeam.

<br>

```cpp  
bool STAADWrapper::CreateNodes(IOSGeometryUIPtr geometry, vector<Node>& nodes) {
...
    try {
        for (auto& node : nodes) {
            _variant_t varX, varY, varZ;
            varX.vt = VT_R8; varX.dblVal = node.x;
            varY.vt = VT_R8; varY.dblVal = node.y;
            varZ.vt = VT_R8; varZ.dblVal = node.z;
            VARIANT result = geometry->AddNode(varX, varY, varZ);
            if (result.vt == VT_I4 && result.lVal > 0) {
                node.staadId = result.lVal;
                nodeIdMap_[node.sapId] = node.staadId;
            }
            counter1= counter1+1;
        }
```
<br>

    Plates de 3 y 4 nodos se asignan con AddPlate.

<br>

```cpp
bool STAADWrapper::CreatePlate(OpenSTAADUI::IOSGeometryUIPtr geometry, int plateId, const std::vector<int>& nodeIds) {
    if (!geometry || nodeIds.size() < 3 || nodeIds.size() > 4) {
        return false;
    }
    try {
        switch (nodeIds.size()) {
        case 3:
            geometry->CreatePlate(plateId, nodeIds[0], nodeIds[1], nodeIds[2]);
            break;
        case 4:
            geometry->CreatePlate(plateId, nodeIds[0], nodeIds[1], nodeIds[2], nodeIds[3]);
            break;
```

<br>

### 3. Asignación de Cargas

    Cargas puntuales → LOAD.

    Cargas distribuidas → UNIFORM LOAD.

## Limitaciones Actuales

  - Plates complejas (>4 nodos) no son soportadas debido a que Staad carece de plates de más de cuatro apoyos. 

  - Soporta hasta 100'000 elementos beam. Los elementos plate son replicados con una numeración que empieza por 100'001.

  - STAAD.Pro debe ser abierto manualmente sin ningún modelo cargado (el código no inicia la instancia del programa por sí mismo debido a las limitantes de la API OpenStaad).

  - Para respetar los nombres de los ejes de SAP2000, se deberá de configurar manualmente Staad con Z up en Configure->General->Global Axes

<br>

## Uso

### Requisitos:

    1. Archivo .$2k de modelo en SAP2000 versión 2022.

    2. STAAD.Pro CONNECT Edition.

    3. Windows 10/11.

### Guía de Uso  
  
    1.  [Abrir Staad.Pro CONNECT Edition manualmente y sin archivo cargado](https://docs.bentley.com/LiveContent/web/STAAD.Pro%20Help-v21/en/GUID-B8FDE305-22B3-44A8-93AE-5649689ABCB8.html)
    2. Ejecuta el programa "SAP2000-a-Staad.exe" como administrador.  
    2. Selecciona tu archivo ".$2K" de SAP2000, arrastra y suelta en la ventana de comandos.
    3. Elige las unidades de longitud y fuerza de tu modelo original.    
    4. ¡Listo! El programa generará el modelo en Staad automáticamente en "C:\temp". Dependiendo del tamaño del modelo será el tiempo de espera.  

## ¿Qué Sigue?

Estamos trabajando en:

    1. Soporte para más tipos de elementos y plates de más de cuatro nodos por medio de "triangular" los plates.

    2. Conversión de comandos de diseño estructural.
