namespace Schmix.Algorithm;

using Schmix.Core;

using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;

public interface IDirectedAcyclicalGraphVertex<T> : IGraphVertex<T> where T : IDirectedAcyclicalGraphVertex<T>
{
    public IReadOnlyCollection<T> Inputs { get; }
    public int OutputCount { get; }
}

public sealed class DirectedAcyclicalGraphSolver<T> : IGraphSolver<T> where T : IDirectedAcyclicalGraphVertex<T>
{
    private readonly HashSet<int> mFinishedVertices, mStack;
    private LinkedList<T>? mResult;

    public DirectedAcyclicalGraphSolver()
    {
        mFinishedVertices = new HashSet<int>();
        mStack = new HashSet<int>();
        mResult = null;
    }

    [MemberNotNull(nameof(mResult))]
    private void Reset()
    {
        mFinishedVertices.Clear();
        mResult = new LinkedList<T>();
    }

    private void ProcessVertex(T vertex)
    {
        var stringified = vertex.ToString();
        if (mFinishedVertices.Contains(vertex.VertexID))
        {
            Log.Trace($"Vertex {stringified} already processed - skipping");
            return;
        }

        var inputs = vertex.Inputs;
        Log.Trace($"Processing vertex {stringified} ({inputs.Count} inputs)");

        if (mStack.Contains(vertex.VertexID))
        {
            Log.Error($"Cyclical dependency detected! Skipping duplicate vertex: {stringified}");
            return;
        }

        mStack.Add(vertex.VertexID);
        foreach (var input in inputs)
        {
            ProcessVertex(input);
        }

        mStack.Remove(vertex.VertexID);

        Log.Trace($"Vertex {stringified} done processing - adding to end of list");
        mResult!.AddLast(vertex);
        mFinishedVertices.Add(vertex.VertexID);
    }

    public IReadOnlyCollection<T> Solve(IReadOnlyCollection<T> graph)
    {
        Reset();
        Log.Trace($"Solving directed acyclical graph with {graph.Count} vertices");

        foreach (var vertex in graph)
        {
            int outputCount = vertex.OutputCount;
            if (outputCount > 0)
            {
                continue;
            }

            Log.Trace($"Output vertex: {vertex}");
            ProcessVertex(vertex);
        }

        Log.Trace("Done solving directed acyclical graph");
        return mResult;
    }
}
