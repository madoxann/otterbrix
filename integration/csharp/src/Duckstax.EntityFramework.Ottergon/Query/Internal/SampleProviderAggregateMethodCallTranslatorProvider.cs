﻿namespace Duckstax.EntityFramework.Ottergon.Query.Internal;

using Microsoft.EntityFrameworkCore.Query;
using Microsoft.EntityFrameworkCore.Sqlite.Query.Internal;

public class SampleProviderAggregateMethodCallTranslatorProvider : RelationalAggregateMethodCallTranslatorProvider
{
    private readonly SqliteAggregateMethodCallTranslatorProvider sqliteAggregateMethodCallTranslatorProvider;
    public SampleProviderAggregateMethodCallTranslatorProvider(RelationalAggregateMethodCallTranslatorProviderDependencies dependencies)
    : base(dependencies)
    {
        sqliteAggregateMethodCallTranslatorProvider = new(dependencies);
    }
}
