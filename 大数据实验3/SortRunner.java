
import org.apache.hadoop.io.Text;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.mapreduce.Mapper;
import java.io.IOException;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Partitioner;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.util.GenericOptionsParser;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.fs.FileSystem;


 


public class SortRunner {
    public static class Partition extends Partitioner<IntWritable, IntWritable> {
 
        @Override
        public int getPartition(IntWritable key, IntWritable value,
                                int numPartitions) {
            int MaxNumber = 65223;
            int bound = MaxNumber / numPartitions + 1;
            int keynumber = key.get();
            for (int i = 0; i < numPartitions; i++) {
                if (keynumber < bound * i && keynumber >= bound * (i - 1))
                    return i - 1;
            }
            return 0;
        }
    }
	
    public static class SortMapper extends Mapper<Object, Text, IntWritable, IntWritable>{
 
		private static IntWritable data = new IntWritable();
	 
		public void map(Object key, Text value, Context context)
				throws IOException, InterruptedException {
			String line = value.toString();
	 
			data.set(Integer.parseInt(line));
	 
			context.write(data, new IntWritable(1));
	 
		}
	}

    public static class SortReducer extends Reducer<IntWritable, IntWritable, IntWritable, IntWritable>{
		private static IntWritable linenum = new IntWritable(1);
	 
		public void reduce(IntWritable key, Iterable<IntWritable> values,
						   Context context) throws IOException, InterruptedException {
	 
			for (IntWritable val : values) {
	 
				context.write(linenum, key);
	 
				linenum = new IntWritable(linenum.get() + 1);
			}
	 
		}
	}
    public static void main(String[] args) throws Exception {
        // TODO Auto-generated method stub
          final String OUTPUT_PATH = "output2";     
        Configuration conf = new Configuration();
        // conf.set("fs.defaultFS", "hdfs://localhost:9000");

        Path path = new Path(OUTPUT_PATH);  

        //加载配置文件
        FileSystem fileSystem = path.getFileSystem(conf);

        //输出目录若存在则删除
        if (fileSystem.exists(new Path(OUTPUT_PATH))) 
        {  
           fileSystem.delete(new Path(OUTPUT_PATH),true);  
        }  

        //指定输入输出目录
        String[] otherArgs = new String[]{"input2","output2"};
        if (otherArgs.length != 2) 
        {
            System.err.println("路径出错");
            System.exit(2);
        }

        Job job = new Job(conf, "SortRunner");
        job.setJarByClass(SortRunner.class);
        job.setMapperClass(SortMapper.class);
        job.setPartitionerClass(Partition.class);
        job.setReducerClass(SortReducer.class);
        job.setOutputKeyClass(IntWritable.class);
        job.setOutputValueClass(IntWritable.class);
        FileInputFormat.addInputPath(job, new Path(otherArgs[0]));  //FileInputFormat指将输入的文件（若大于64M）进行切片划分，每个split切片对应一个Mapper任务
        FileOutputFormat.setOutputPath(job, new Path(otherArgs[1]));
        System.exit(job.waitForCompletion(true) ? 0 : 1);
    }
}