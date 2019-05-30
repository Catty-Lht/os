
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import java.io.IOException;

public class Merge {


    //Map类，继承自Mapper类--一个抽象类
    public static class Map extends Mapper<Object, Text, Text, Text> 
    {
        private static Text text = new Text();

        //重写map方法
        public void map(Object key, Text value, Context content) throws IOException, InterruptedException 
        {
            text = value;

          //底层通过Context content传递信息（即key value）
            content.write(text, new Text(""));
        }
    }

    //Reduce类，继承自Reducer类--一个抽象类
    public static class Reduce extends Reducer<Text, Text, Text, Text> 
    {
        public void reduce(Text key, Iterable<Text> values, Context context) throws IOException, InterruptedException 
        {
            //对于所有的相同的key，只写入一个，相当于对于所有Iterable<Text> values，只执行一次write操作
            context.write(key, new Text(""));
        }
    }



    //main方法
    public static void main(String[] args) throws Exception {

        final String OUTPUT_PATH = "output1";     
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
        String[] otherArgs = new String[]{"input1","output1"};
        if (otherArgs.length != 2) 
        {
            System.err.println("路径出错");
            System.exit(2);
        }

        //一些初始化
        Job job = Job.getInstance(conf,"Merge");
        job.setJarByClass(Merge.class);
        job.setMapperClass(Map.class);  //初始化为自定义Map类
        job.setReducerClass(Reduce.class);  //初始化为自定义Reduce类
        job.setOutputKeyClass(Text.class);  //指定输出的key的类型，Text相当于String类
        job.setOutputValueClass(Text.class);  //指定输出的Value的类型，Text相当于String类

        FileInputFormat.addInputPath(job, new Path(otherArgs[0]));  //FileInputFormat指将输入的文件（若大于64M）进行切片划分，每个split切片对应一个Mapper任务
        FileOutputFormat.setOutputPath(job, new Path(otherArgs[1]));
        System.exit(job.waitForCompletion(true) ? 0 : 1);

    }
}