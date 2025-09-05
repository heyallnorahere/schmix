namespace Schmix.UI;

using ImGuiNET;
using imnodesNET;

using Schmix.Algorithm;
using Schmix.Core;
using Schmix.Extension;

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Linq;
using System.Numerics;

public static class Rack
{
    private struct EndpointMeta
    {
        public Cable? Cable;
        public int ID;
    }

    private struct NodeMeta : IDirectedAcyclicalGraphVertex<NodeMeta>
    {
        IReadOnlyCollection<NodeMeta> IDirectedAcyclicalGraphVertex<NodeMeta>.Inputs
        {
            get
            {
                var inputs = new List<NodeMeta>();
                foreach (var endpoint in Inputs.Values)
                {
                    var cable = endpoint.Cable;
                    if (cable is null)
                    {
                        continue;
                    }

                    var source = cable.Source;
                    var sourceNode = source.Instance;
                    int sourceID = sourceNode.ID;

                    var meta = sNodes[sourceID];
                    inputs.Add(meta);
                }

                return inputs;
            }
        }

        int IDirectedAcyclicalGraphVertex<NodeMeta>.OutputCount
        {
            get
            {
                int count = 0;
                foreach (var endpoint in Outputs.Values)
                {
                    var cable = endpoint.Cable;
                    if (cable is null)
                    {
                        continue;
                    }

                    count++;
                }

                return count;
            }
        }

        int IGraphVertex<NodeMeta>.VertexID => Instance.ID;

        public Node Instance;
        public Dictionary<int, EndpointMeta> Inputs, Outputs;
    }

    private static readonly Dictionary<int, NodeMeta> sNodes = new Dictionary<int, NodeMeta>();
    private static readonly Dictionary<int, Cable> sCables = new Dictionary<int, Cable>();
    private static readonly Dictionary<int, Cable.Endpoint> sEndpointNodeMap = new Dictionary<int, Cable.Endpoint>();

    private static IEnumerable<int>? sNodeUpdateOrder = null;
    private static Dictionary<int, Vector2> sNodeSizes = new Dictionary<int, Vector2>();

    private static int sChannels = -1;
    private static int sSampleRate = -1;
    private static int sSamplesRequested = -1;

    public static int Channels
    {
        get => sChannels;
        set
        {
            if (sSamplesRequested >= 0)
            {
                throw new InvalidOperationException("Attempted to set channel count while processing data!");
            }

            sChannels = value;
        }
    }

    public static int SampleRate
    {
        get => sSampleRate;
        set
        {
            if (sSamplesRequested >= 0)
            {
                throw new InvalidOperationException("Attempted to set sample rate while processing data!");
            }

            sSampleRate = value;
        }
    }

    public static int SamplesRequested
    {
        get
        {
            if (sSamplesRequested < 0)
            {
                throw new InvalidOperationException("Rack is not processing!");
            }

            return sSamplesRequested;
        }
    }

    [MemberNotNull(nameof(sNodeUpdateOrder))]
    private static void RegenerateUpdateOrder()
    {
        Log.Info("Regenerating update order...");

        var solver = new DirectedAcyclicalGraphSolver<NodeMeta>();
        sNodeUpdateOrder = solver.Solve(sNodes.Values).Select(meta => meta.Instance.ID);
    }

    public static void AddNode(Node node)
    {
        Log.Info($"Adding node: {node.Name}");

        var meta = new NodeMeta
        {
            Instance = node,

            Inputs = new Dictionary<int, EndpointMeta>(),
            Outputs = new Dictionary<int, EndpointMeta>()
        };

        sNodes.Add(node.ID, meta);

        RegenerateUpdateOrder();
    }

    public static int AddModule(Module module)
    {
        var node = new ModuleNode(module);
        AddNode(node);

        return node.ID;
    }

    private static void VerifyEndpointListContainsIndex(IDictionary<int, EndpointMeta> list, int index)
    {
        if (list.ContainsKey(index))
        {
            return;
        }

        list.Add(index, new EndpointMeta
        {
            Cable = null,
            ID = -1
        });
    }

    private static void AddCableToEndpointList(IDictionary<int, EndpointMeta> list, int index, Cable cable)
    {
        VerifyEndpointListContainsIndex(list, index);

        var meta = list[index];
        meta.Cable = cable;
        list[index] = meta;
    }

    public static int AddCable(Cable.Endpoint source, Cable.Endpoint destination)
    {
        var sourceNode = source.Instance;
        var sourceMeta = sNodes[sourceNode.ID];

        var destinationNode = destination.Instance;
        var destinationMeta = sNodes[destinationNode.ID];

        EndpointMeta meta;
        if (sourceMeta.Outputs.TryGetValue(source.Index, out meta) && meta.Cable is not null)
        {
            Log.Warn($"Attempted to add two cables to node {sourceNode.Name} output {source.Index}");
            return -1;
        }

        if (destinationMeta.Inputs.TryGetValue(destination.Index, out meta) && meta.Cable is not null)
        {
            Log.Warn($"Attempted to add two cables to node {destinationNode.Name} input {destination.Index}");
            return -1;
        }

        Log.Info($"Adding cable from node {sourceNode.Name} output {source.Index} to node {destinationNode.Name} input {destination.Index}");
        var cable = new Cable(source, destination);

        int id = cable.ID;
        sCables.Add(id, cable);

        AddCableToEndpointList(sourceMeta.Outputs, source.Index, cable);
        AddCableToEndpointList(destinationMeta.Inputs, destination.Index, cable);

        RegenerateUpdateOrder();

        return id;
    }

    private static void RemoveCableFromEndpointList(IDictionary<int, EndpointMeta> list, int index)
    {
        var meta = list[index];
        if (meta.ID < 0)
        {
            list.Remove(index);
            return;
        }

        meta.Cable = null;
        list[index] = meta;
    }

    public static void RemoveCable(int id)
    {
        if (!sCables.ContainsKey(id))
        {
            Log.Debug($"Attempted to remove nonexistent cable {id} - skipping");
            return;
        }

        Log.Debug($"Removing cable: {id}");

        var cable = sCables[id];
        var source = cable.Source;
        var destination = cable.Destination;

        int sourceID = source.Instance.ID;
        int destinationID = destination.Instance.ID;

        RemoveCableFromEndpointList(sNodes[sourceID].Outputs, source.Index);
        RemoveCableFromEndpointList(sNodes[destinationID].Inputs, destination.Index);

        sCables.Remove(id);
    }

    public static void RemoveNode(int id)
    {
        if (!sNodes.ContainsKey(id))
        {
            Log.Debug($"Attempted to remove nonexistent node {id} - skipping");
            return;
        }

        Log.Debug($"Removing node: {id}");

        var meta = sNodes[id];
        var inputMeta = meta.Inputs.Values.ToArray();
        var outputMeta = meta.Outputs.Values.ToArray();
        var endpointMeta = inputMeta.Concat(outputMeta);

        foreach (var endpoint in endpointMeta)
        {
            var cable = endpoint.Cable;
            if (cable is not null)
            {
                RemoveCable(cable.ID);
            }

            if (endpoint.ID >= 0)
            {
                sEndpointNodeMap.Remove(endpoint.ID);
            }
        }

        meta.Instance.Dispose();
        sNodes.Remove(id);

        RegenerateUpdateOrder();
    }

    public static void Clear()
    {
        Log.Debug("Clearing rack");

        var ids = sNodes.Keys.ToArray();
        foreach (int id in ids)
        {
            RemoveNode(id);
        }

        if (sCables.Count > 0)
        {
            Log.Warn($"{sCables.Count} dangling cables - cleaning up");
            sCables.Clear();
        }

        if (sEndpointNodeMap.Count > 0)
        {
            Log.Warn($"{sEndpointNodeMap.Count} dangling endpoints - cleaning up");
            sEndpointNodeMap.Clear();
        }
    }

    private static void UpdateSanityChecks()
    {
        if (sChannels < 0)
        {
            throw new InvalidOperationException("\"Rack.Channels\" not set!");
        }

        if (sSampleRate < 0)
        {
            throw new InvalidOperationException("\"Rack.SampleRate\" not set!");
        }
    }

    private static void GetSampleRequest()
    {
        sSamplesRequested = 0;
        foreach (var meta in sNodes.Values)
        {
            var node = meta.Instance;
            if (node is not ModuleNode moduleNode)
            {
                continue;
            }

            var module = moduleNode.Instance;
            int samplesRequested = module.SamplesRequested;

            if (samplesRequested > sSamplesRequested)
            {
                sSamplesRequested = samplesRequested;
            }
        }
    }

    public static void Update()
    {
        UpdateSanityChecks();

        try
        {
            GetSampleRequest();

            if (sNodeUpdateOrder is null)
            {
                RegenerateUpdateOrder();
            }

            foreach (int id in sNodeUpdateOrder)
            {
                var meta = sNodes[id];
                var node = meta.Instance;

                var inputNames = node.Inputs;
                var outputNames = node.Outputs;

                var inputs = new Cable?[inputNames.Count];
                var outputs = new Cable?[outputNames.Count];

                for (int i = 0; i < inputs.Length; i++)
                {
                    Cable? cable = null;
                    if (meta.Inputs.TryGetValue(i, out EndpointMeta endpoint))
                    {
                        cable = endpoint.Cable;
                    }

                    inputs[i] = cable;
                }

                for (int i = 0; i < outputs.Length; i++)
                {
                    Cable? cable = null;
                    if (meta.Outputs.TryGetValue(i, out EndpointMeta endpoint))
                    {
                        cable = endpoint.Cable;
                    }

                    cable?.ResetSignal();
                    outputs[i] = cable;
                }

                node.Update(inputs, outputs);
            }
        }
        catch (Exception ex)
        {
            Log.Error($"Error processing audio: {ex}");
        }

        sSamplesRequested = -1;
    }

    private static int GetNodeInputID(NodeMeta meta, int index)
    {
        var endpoint = new Cable.Endpoint(meta.Instance, index);

        if (meta.Inputs.TryGetValue(index, out var endpointMeta))
        {
            if (endpointMeta.ID < 0)
            {
                endpointMeta.ID = meta.Instance.GetInputID(index);
                meta.Inputs[index] = endpointMeta;

                sEndpointNodeMap.Add(endpointMeta.ID, endpoint);
            }

            return endpointMeta.ID;
        }

        int id = meta.Instance.GetInputID(index);
        meta.Inputs.Add(index, new EndpointMeta
        {
            Cable = null,
            ID = id
        });

        sEndpointNodeMap.Add(id, endpoint);
        return id;
    }

    private static int GetNodeOutputID(NodeMeta meta, int index)
    {
        var endpoint = new Cable.Endpoint(meta.Instance, index);

        if (meta.Outputs.TryGetValue(index, out var endpointMeta))
        {
            if (endpointMeta.ID < 0)
            {
                endpointMeta.ID = meta.Instance.GetOutputID(index);
                meta.Outputs[index] = endpointMeta;

                sEndpointNodeMap.Add(endpointMeta.ID, endpoint);
            }

            return endpointMeta.ID;
        }

        int id = meta.Instance.GetOutputID(index);
        meta.Outputs.Add(index, new EndpointMeta
        {
            Cable = null,
            ID = id
        });

        sEndpointNodeMap.Add(id, endpoint);
        return id;
    }

    private static int sContextNode = -1;
    public static void Render(ref bool show)
    {
        if (!show)
        {
            return;
        }

        if (ImGui.Begin("Rack", ref show))
        {
            int hoveredNode = 0;
            bool nodeHovered = imnodes.IsNodeHovered(ref hoveredNode);

            imnodes.PushAttributeFlag(AttributeFlags.EnableLinkDetachWithDragClick);
            imnodes.BeginNodeEditor();

            bool focused = ImGui.IsWindowFocused(ImGuiFocusedFlags.RootAndChildWindows);
            bool editorHovered = imnodes.IsEditorHovered();
            bool rightMouseClicked = ImGui.IsMouseClicked(ImGuiMouseButton.Right);

            ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, Vector2.One * 8f);
            if (focused && editorHovered && rightMouseClicked)
            {
                if (nodeHovered)
                {
                    sContextNode = hoveredNode;
                    ImGui.OpenPopup("node-context");
                }
                else
                {
                    ImGui.OpenPopup("global-context");
                }
            }

            if (ImGui.BeginPopup("node-context"))
            {
                if (ImGui.MenuItem("Remove"))
                {
                    RemoveNode(sContextNode);
                }

                ImGui.EndPopup();
            }

            if (ImGui.BeginPopup("global-context"))
            {
                if (ImGui.BeginMenu("Add"))
                {
                    var plugins = Plugin.Plugins;
                    int pluginIndex = 0;

                    foreach ((var name, var plugin) in plugins)
                    {
                        ImGui.PushID($"plugin-{pluginIndex}");

                        if (ImGui.MenuItem(name))
                        {
                            Log.Info($"Adding module from plugin {name}");

                            var module = plugin.Instantiate();
                            Rack.AddModule(module);
                        }

                        ImGui.PopID();
                    }

                    ImGui.EndMenu();
                }

                ImGui.EndPopup();
            }

            ImGui.PopStyleVar();

            foreach ((int id, var meta) in sNodes)
            {
                var node = meta.Instance;

                imnodes.BeginNode(id);

                imnodes.BeginNodeTitleBar();
                ImGui.TextUnformatted(node.Name);
                imnodes.EndNodeTitleBar();

                var inputNames = node.Inputs;
                var outputNames = node.Outputs;

                for (int i = 0; i < inputNames.Count; i++)
                {
                    int inputID = GetNodeInputID(meta, i);

                    imnodes.BeginInputAttribute(inputID);
                    ImGui.TextUnformatted(inputNames[i]);
                    imnodes.EndInputAttribute();
                }

                node.Render();

                Vector2 nodeSize;
                if (!sNodeSizes.TryGetValue(id, out nodeSize))
                {
                    nodeSize = Vector2.One * 10f;
                }

                for (int i = 0; i < outputNames.Count; i++)
                {
                    int outputID = GetNodeOutputID(meta, i);
                    imnodes.BeginOutputAttribute(outputID);

                    var name = outputNames[i];
                    var nameSize = ImGui.CalcTextSize(name);
                    ImGui.Indent(nodeSize.X - nameSize.X);
                    ImGui.TextUnformatted(name);

                    imnodes.EndOutputAttribute();
                }

                imnodes.EndNode();

                nodeSize = ImGui.GetItemRectSize();
                sNodeSizes[id] = nodeSize;
            }

            foreach ((int id, var cable) in sCables)
            {
                var source = cable.Source;
                var sourceMeta = sNodes[source.Instance.ID];

                var destination = cable.Destination;
                var destinationMeta = sNodes[destination.Instance.ID];

                int sourceID = GetNodeOutputID(sourceMeta, source.Index);
                int destinationID = GetNodeInputID(destinationMeta, destination.Index);

                imnodes.Link(id, sourceID, destinationID);
            }

            imnodes.EndNodeEditor();

            int startID = 0;
            int endID = 0;
            if (imnodes.IsLinkCreated(ref startID, ref endID))
            {
                var source = sEndpointNodeMap[startID];
                var destination = sEndpointNodeMap[endID];

                AddCable(source, destination);
            }

            int cableID = 0;
            if (imnodes.IsLinkDestroyed(ref cableID))
            {
                RemoveCable(cableID);
            }

            imnodes.PopAttributeFlag();
        }

        ImGui.End();
    }
}
