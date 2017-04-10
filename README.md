## Welcome to libWave

A C++ library for asynchronous functional programming.

All you need is the `>>=` operator.

### Start with the loop

```C++
int main()
{
  using namespace wave;
  loop main_loop;
  idle{} >>= { std::cout << "Hello World!"; };
  return 0;
}
```

### Idling

```C++
idle{2} 
>>= ${ std::cout << "This message is twice!"; };
```

### Timer

```C++
timer{1000} 
>>= ${ std::cout << "One second passed!"; };
```

### Composing

```C++
idle{} 
>>= ${ return 1000; }
>>= $(int timeout) { return timer{timeout}; }
>>= ${ std::cout << "Maybe one second passed!"; };
